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

/* Common PCF8574 backpack: P0=RS, P1=RW, P2=E, P3=backlight,
 * P4..P7=D4..D7. Change these masks for a differently wired backpack. */
#define LCD_RS_MASK        (0x01U)
#define LCD_ENABLE_MASK    (0x04U)
#define LCD_BACKLIGHT_MASK (0x08U)

/* Address 0 scans only the PCF8574 range 0x20..0x27; requires one responder.
 * Otherwise supply the known 7-bit address. Call once, before the scheduler.
 * Uses polling with bounded timeouts; no SCB ISR is required. */
lcd_result_t lcd1602_init(CySCB_Type *base,
                        const cy_stc_scb_i2c_config_t *config,
                        uint32_t clock_hz, uint8_t address);

/* Call after successful initialization; cursor coordinates are zero-based. */
lcd_result_t lcd1602_set_cursor(uint8_t row, uint8_t column);
/* Write a NUL-terminated string that fits on the selected row. */
lcd_result_t lcd1602_write(const char *text);

/* Inspect these in the debugger; this driver does not use UART. */
extern volatile uint8_t lcd1602_address;
extern volatile uint32_t lcd1602_bus_hz;
extern volatile cy_en_scb_i2c_status_t lcd1602_last_i2c_status;

#endif
