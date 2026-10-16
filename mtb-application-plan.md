# ModusToolbox application plan

- Project: E84_I2C_LCD_Hello_World
- Location: C:\Users\yuliya\Documents\PSOC_proj\E84_I2C_LCD_Hello_World
- Board: KIT_PSE84_AI (application BSP APP_KIT_PSE84_AI)
- Template: mtb-example-psoc-edge-empty-app
- Output: `Hello World` on the first row of a 1602 HD44780-compatible LCD.
- Interface: PCF8574T I2C backpack, polling transfers at up to 100 kHz.
- Default firmware bus: generated SCB5 display controller aliases, P17.0 SCL / P17.1 SDA.
- Address: detect a single responder at 0x20..0x27; configurable fixed 7-bit address.
- Hardware configuration and wiring: user will configure themselves.
- Implementation: LCD initialization, cursor positioning and text output in proj_cm33_ns.
- Dependencies: standard template BSP, PDL and RTOS; no additional LCD library.
- Secure CM33 and CM55 template startup retained.
- Verification: compile the complete application and generate VS Code workspace.
- Physical display verification remains pending user hardware setup and programming.

See docs/LCD_SETUP.md for hardware configuration, backpack mapping and diagnostics.

## Verification result (2026-10-13)
- Complete application build: PASS (CM33 secure, CM33 non-secure, CM55; signing and image merge succeeded).
- VS Code workspace generation: PASS.
- Board programming and physical LCD output: not performed; user hardware configuration pending.
