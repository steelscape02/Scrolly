# Scrolly

> Scrolly is currently in a development stage. In the final version, Scrolly will be a completely enclosed product, using bluetooth for programming, with serial as a backup.

A solution to boredom... I think. Scrolly is a scrolling text display designed around an STM32 Nucleo L476RG development board.
Scrolly can be programmed over UART, and uses an I2C bus to handle both the display and an EEPROM IC for storage.

## How to use

Scrolly is designed to be simple to use. However, at this stage of development programming is only available over UART. TO perform this program, follow the steps below:

### Setup communication

1. Connect the Nucleo board to your computer
2. Install the [STLink USB drivers](https://www.st.com/en/development-tools/stsw-link009.html)
3. Open a serial communication tool like PuTTY
4. Configure the connection to the Nucleo board using the serial port chosen by the STLink USB driver and a 9600 baud rate

### Communicate with the board

Now that you are connected to the board, you can use the two available commands to perform add and erase operations:

#### `ADD`

Ex: `ADD Hello World`

Adds desired text to the screen, overwriting the previous text

#### `ERASE`

Ex: `ERASE`

Erases the text on the screen. Backlight will remain on.

## How it works

From boot, the below process shows how this system works

1. Reads EEPROM chip to retrieve message if present
2. Enters an infinite `while` loop, waiting for motion detection or user input
3. If motion is detected, the backlight turns on and the text (if available) starts to display
4. If user input is detected, the command is interpreted and performed.

## How to build it

If you are interested in building Scrolly for yourself, you will need:

- 1x STM32 Nucleo L476RG Development board
- 1x HCSR502 PIR Motion Sensor, or equivalent
- 1x 16x2 I2C LCD display
- 1x [24LC08B 8K I2C EEPROM IC](https://ww1.microchip.com/downloads/en/DeviceDoc/24AA08-24LC08B-24FC08-8K-I2C-Serial-EEPROM-20001710N.pdf) (For this version, use a THT model)
- ~20 jumper wires

Connect these components in the configuration shown below. *Please note that the WP (Write Protect) pin on the EEPROM is connected to GND to disable write protection.*

<img width="3000" height="2566" alt="circuit_image" src="https://github.com/user-attachments/assets/13ec2e5b-10f9-4fa1-bfca-b8838bce3ecd" />
