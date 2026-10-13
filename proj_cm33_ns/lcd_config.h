#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#include "cybsp.h"

/* Change these aliases if you choose another bus in Device Configurator.
 * Current generated routing: SCB5, P17.0=SCL, P17.1=SDA (3.3 V labels).
 * Configure physical wiring, supply and pull-ups yourself before programming. */
#define LCD_I2C_HW        CYBSP_I2C_DISPLAY_CONTROLLER_HW
#define LCD_I2C_CONFIG    CYBSP_I2C_DISPLAY_CONTROLLER_config
#define LCD_CLOCK_GROUP  CYBSP_I2C_CONTROLLER_3V3_CLK_DIV_GRP_NUM
#define LCD_CLOCK_TYPE   CYBSP_I2C_CONTROLLER_3V3_CLK_DIV_HW
#define LCD_CLOCK_NUMBER CYBSP_I2C_CONTROLLER_3V3_CLK_DIV_NUM

/* 0 = detect a single PCF8574 in 0x20..0x27. Set e.g. 0x27 for a fixed address.
 * Address detection finds an ACK; it cannot identify the responding device. */
#define LCD_I2C_ADDRESS (0U)

#endif
