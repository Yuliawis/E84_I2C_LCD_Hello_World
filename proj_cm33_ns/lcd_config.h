#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#include "cybsp.h"

/* Default bus: SCB5, P17.0 SCL and P17.1 SDA, with 3.3 V logic.
 * Match these aliases to Device Configurator if you change buses.
 * Check wiring, power and pull-up voltage before programming. */
#define LCD_I2C_HW        CYBSP_I2C_DISPLAY_CONTROLLER_HW
#define LCD_I2C_CONFIG    CYBSP_I2C_DISPLAY_CONTROLLER_config
#define LCD_CLOCK_GROUP  CYBSP_I2C_CONTROLLER_3V3_CLK_DIV_GRP_NUM
#define LCD_CLOCK_TYPE   CYBSP_I2C_CONTROLLER_3V3_CLK_DIV_HW
#define LCD_CLOCK_NUMBER CYBSP_I2C_CONTROLLER_3V3_CLK_DIV_NUM

/* Use 0 to find one responder at 0x20..0x27, or set a fixed 7-bit address.
 * A response confirms an address is occupied, not that the device is an LCD. */
#define LCD_I2C_ADDRESS (0U)

#endif
