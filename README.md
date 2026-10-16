# E84 I2C LCD Hello World

Target: KIT_PSE84_AI, based on Infineon's PSOC Edge Empty Application.
At startup the CM33 non-secure application initializes a PCF8574T backpack,
clears the 1602 LCD, enables the backlight and writes `Hello World` on row 1.
Row 2 starts blank; both rows are used by the lyric sequence. No additional middleware is required.

## Your hardware configuration

Firmware currently uses the BSP's SCB5 display I2C aliases: P17.0 SCL, 
P17.1 SDA (3.3 V bus). If you use another bus, configure pins, clocks and 
access in Device Configurator and update `proj_cm33_ns/lcd_config.h` to match the 
generated aliases.

Use common ground and compatible voltage levels. If the LCD backpack runs
at 5 V with pull-ups to 5 V, use a bidirectional I2C level shifter between
the 3.3 V board bus and the backpack. Supply the LCD according to its module
specifications; the PCF8574's voltage range alone does not establish the
LCD module's required voltage. Adjust the contrast potentiometer after power-up.

`LCD_I2C_ADDRESS=0` probes 7-bit addresses 0x20 through 0x27 without writing
port data, and requires exactly one responding device. Set a fixed address
(for example 0x27) if other devices occupy that range. An ACK does not prove
that the device is an LCD. PCF8574A devices use a different range.

Assumed backpack connections: P0=RS, P1=RW, P2=Enable, P3=backlight active high,
P4..P7=LCD D4..D7. RW remains low. Different backpack wiring needs driver changes.

## Lyric display sequence

After Hello World (2 seconds), a FreeRTOS task shows the user-supplied excerpt
of Love Me Not once. Each page uses two rows of at most 16 characters and remains
for 2200 ms. These are approximate delays, not measured song or beat timings.
No audio playback or beat detection is implemented. Start the excerpt when the
first lyric appears; reset the board to replay.

Edit LCD_HELLO_HOLD_MS and lyric_pages[].hold_ms in proj_cm33_ns/main.c to adjust
the start delay and individual page durations. Rows are space-padded to remove
previous text. If an I2C update fails, the task stops and leaves lcd_status
available for inspection.
