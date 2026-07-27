# Scrolly

> Scrolly is currently in a development stage. In the final version, Scrolly will be a completely enclosed product, using bluetooth for programming, with serial as a backup.

A solution to boredom... I think. Scrolly is a scrolling text display designed around an STM32 Nucleo L476RG development board.
Scrolly can be programmed over UART, and uses an I2C bus to handle both the display and an EEPROM IC for storage.

## How it works

From boot, the below process shows how this system works

1. Reads EEPROM chip to retrieve message if present
2. Enters an infinite `while` loop, waiting for motion detection or user input
3. If motion is detected, the backlight turns on and the text (if available) starts to display
4. If user input is detected, the command is interpreted and performed.
