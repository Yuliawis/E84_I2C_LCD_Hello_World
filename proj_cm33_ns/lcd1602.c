#include "lcd1602.h"

#define LCD_TIMEOUT_MS (10U)

static CySCB_Type *lcd_bus;
static bool lcd_ready;
static cy_stc_scb_i2c_context_t lcd_context;
volatile uint8_t lcd1602_address;
volatile uint32_t lcd1602_bus_hz;
volatile cy_en_scb_i2c_status_t lcd1602_last_i2c_status;

static lcd_result_t finish_transfer(cy_en_scb_i2c_status_t status)
{
    cy_en_scb_i2c_status_t stop = Cy_SCB_I2C_MasterSendStop(
        lcd_bus, LCD_TIMEOUT_MS, &lcd_context);
    /* Allow the target to release the bus after STOP, with a finite timeout. */
    uint32_t remaining = 1000U;
    while (Cy_SCB_I2C_IsBusBusy(lcd_bus) && remaining != 0U)
    {
        --remaining;
        Cy_SysLib_DelayUs(1U);
    }
    lcd1602_last_i2c_status = (status == CY_SCB_I2C_SUCCESS) ? stop : status;
    return (status == CY_SCB_I2C_SUCCESS && stop == CY_SCB_I2C_SUCCESS &&
            !Cy_SCB_I2C_IsBusBusy(lcd_bus)) ? LCD_OK : LCD_ERROR_BUS;
}

static lcd_result_t write_nibble(uint8_t nibble, uint8_t rs)
{
    uint8_t value = (uint8_t)((nibble << 4U) | LCD_BACKLIGHT_MASK | rs);
    const uint8_t pulse[] = {value, (uint8_t)(value | LCD_ENABLE_MASK), value};
    cy_en_scb_i2c_status_t status = Cy_SCB_I2C_MasterSendStart(
        lcd_bus, lcd1602_address, CY_SCB_I2C_WRITE_XFER,
        LCD_TIMEOUT_MS, &lcd_context);
    for (uint32_t i = 0U; i < sizeof(pulse) && status == CY_SCB_I2C_SUCCESS; ++i)
    {
        status = Cy_SCB_I2C_MasterWriteByte(lcd_bus, pulse[i],
                                          LCD_TIMEOUT_MS, &lcd_context);
    }
    return finish_transfer(status);
}

static lcd_result_t command(uint8_t value)
{
    lcd_result_t result = write_nibble(value >> 4U, 0U);
    if (result == LCD_OK)
    {
        result = write_nibble(value & 0x0FU, 0U);
    }
    /* RW stays low; wait rather than reading the LCD busy flag. */
    Cy_SysLib_DelayUs((value == 0x01U || value == 0x02U) ? 2000U : 50U);
    return result;
}

static lcd_result_t find_address(uint8_t requested)
{
    uint32_t found = 0U;
    uint8_t first = (requested == 0U) ? 0x20U : requested;
    uint8_t last = (requested == 0U) ? 0x27U : requested;
    for (uint32_t address = first; address <= last; ++address)
    {
        cy_en_scb_i2c_status_t status = Cy_SCB_I2C_MasterSendStart(
            lcd_bus, address, CY_SCB_I2C_WRITE_XFER, LCD_TIMEOUT_MS, &lcd_context);
        lcd_result_t finish = finish_transfer(status);
        if (status == CY_SCB_I2C_SUCCESS && finish == LCD_OK)
        {
            lcd1602_address = (uint8_t)address;
            ++found;
        }
        else if (status != CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK ||
                 Cy_SCB_I2C_IsBusBusy(lcd_bus))
        {
            return LCD_ERROR_BUS;
        }
    }
    return (found == 1U) ? LCD_OK :
           ((found == 0U) ? LCD_ERROR_NOT_FOUND : LCD_ERROR_MULTIPLE_DEVICES);
}

lcd_result_t lcd1602_init(CySCB_Type *base,
                        const cy_stc_scb_i2c_config_t *config,
                        uint32_t clock_hz, uint8_t address)
{
    lcd_ready = false;
    if (base == NULL || config == NULL || clock_hz == 0U ||
        (address != 0U && (address < 0x08U || address > 0x77U)) ||
        (config->i2cMode != CY_SCB_I2C_MASTER &&
         config->i2cMode != CY_SCB_I2C_MASTER_SLAVE))
    {
        return LCD_ERROR_CONFIG;
    }
    lcd_bus = base;
    lcd1602_last_i2c_status = Cy_SCB_I2C_Init(base, config, &lcd_context);
    if (lcd1602_last_i2c_status != CY_SCB_I2C_SUCCESS)
    {
        return LCD_ERROR_CONFIG;
    }
    /* The PDL derives SCL timings/filter settings from the actual SCB clock.
     * PCF8574 supports standard mode (100 kHz maximum). */
    lcd1602_bus_hz = Cy_SCB_I2C_SetDataRate(base, 100000U, clock_hz);
    if (lcd1602_bus_hz == 0U || lcd1602_bus_hz > 100000U)
    {
        return LCD_ERROR_CONFIG;
    }
    Cy_SCB_I2C_Enable(base);
    Cy_SysLib_Delay(50U);
    lcd_result_t result = find_address(address);
    if (result != LCD_OK)
    {
        return result;
    }
    /* Datasheet power-on sequence, including recovery from an MCU-only reset. */
    const uint8_t nibbles[] = {0x03U, 0x03U, 0x03U, 0x02U};
    for (uint32_t i = 0U; i < sizeof(nibbles); ++i)
    {
        result = write_nibble(nibbles[i], 0U);
        if (result != LCD_OK)
        {
            return result;
        }
        Cy_SysLib_DelayUs((i == 0U) ? 5000U : 200U);
    }
    /* Four-bit, two-line, 5x8 font; off; clear; increment; on, no cursor. */
    const uint8_t commands[] = {0x28U, 0x08U, 0x01U, 0x06U, 0x0CU};
    for (uint32_t i = 0U; i < sizeof(commands); ++i)
    {
        result = command(commands[i]);
        if (result != LCD_OK)
        {
            return result;
        }
    }
    lcd_ready = true;
    return LCD_OK;
}

/* Row and column are zero-based. HD44780 row 2 begins at DDRAM 0x40. */
lcd_result_t lcd1602_set_cursor(uint8_t row, uint8_t column)
{
    if (!lcd_ready || row >= 2U || column >= 16U)
    {
        return LCD_ERROR_CONFIG;
    }
    return command((uint8_t)(0x80U | (row == 0U ? 0U : 0x40U) | column));
}

/* Caller keeps text within the current row's 16 columns. */
lcd_result_t lcd1602_write(const char *text)
{
    if (!lcd_ready || text == NULL)
    {
        return LCD_ERROR_CONFIG;
    }
    while (*text != '\0')
    {
        uint8_t value = (uint8_t)*text++;
        lcd_result_t result = write_nibble(value >> 4U, LCD_RS_MASK);
        if (result == LCD_OK)
        {
            result = write_nibble(value & 0x0FU, LCD_RS_MASK);
        }
        if (result != LCD_OK)
        {
            return result;
        }
        Cy_SysLib_DelayUs(50U);
    }
    return LCD_OK;
}
