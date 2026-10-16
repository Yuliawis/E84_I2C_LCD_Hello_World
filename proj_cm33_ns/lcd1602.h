#ifndef LCD1602_H
#define LCD1602_H

#include "cy_pdl.h"

typedef enum
{
    LCD_OK = 0,
    LCD_ERROR_CONFIG,
    LCD_ERROR_BUS,
    LCD_ERROR_NOT_FOUND,
    LCD_ERROR_MULTIPLE_DEVICES
} lcd_result_t;

/* Backpack wiring: P0=RS, P1=RW, P2=Enable, P3=backlight, P4..P7=D4..D7.
 * The driver assumes this mapping; adapt it if your module differs. */
#define LCD_RS_MASK        (0x01U)
#define LCD_ENABLE_MASK    (0x04U)
#define LCD_BACKLIGHT_MASK (0x08U)

/* Initialize once before starting FreeRTOS. Transfers use polling and finite timeouts.
 * Address 0 requires one responder at 0x20..0x27; other values select a fixed address.
 * No I2C interrupt handler is needed. */
lcd_result_t lcd1602_init(CySCB_Type *base,
                        const cy_stc_scb_i2c_config_t *config,
                        uint32_t clock_hz, uint8_t address);

/* Select a row (0..1) and column (0..15) after initialization. */
lcd_result_t lcd1602_set_cursor(uint8_t row, uint8_t column);
/* Write a NUL-terminated string; keep it within the remaining columns. */
lcd_result_t lcd1602_write(const char *text);

/* Debugger diagnostics: detected address, bus speed and last transfer status. */
extern volatile uint8_t lcd1602_address;
extern volatile uint32_t lcd1602_bus_hz;
extern volatile cy_en_scb_i2c_status_t lcd1602_last_i2c_status;

#endif
