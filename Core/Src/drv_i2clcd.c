/*
 * drv_i2clcd.c
 *
 *  Created on: Jul 18, 2026
 *      Author: nicho
 */

#include "drv_i2clcd.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include <stdint.h>

// 1602 message bit numbers
#define DC_BIT 0 // Data/Command bit (register select bit)
#define EN_BIT 2 // Enable bit
#define BL_BIT 3 // Backlight bit
#define D4_BIT 4 // Data 4 bit
#define D5_BIT 5 // Data 5 bit
#define D6_BIT 6 // Data 6 bit
#define D7_BIT 7 // Data 7 bit

static uint8_t lcd_port_state = 0x00;
static bool lcd_backlight_enabled = false;

/**
 * @brief Write a byte to the LCD via I2C
 * @param hi2c1: Pointer to I2C handle (e.g., &hi2c1)
 * @param I2C_ADDR: I2C address of the LCD
 * @param data: 8-bit data to send to the LCD
 * @retval None
 */
static void CharLCD_Write_Port(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, uint8_t data) {
	HAL_I2C_Master_Transmit(hi2c1, I2C_ADDR << 1, &data, 1, 100);
}

/**
 * @brief Initialize LCD in 4-bit mode via I2C
 * @param None
 * @retval None
 */
void CharLCD_Init(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR) {
	HAL_Delay(50); // Wait for LCD power-on reset (>40ms)
	CharLCD_Write_Nibble(hi2c1, 0x03, 0, false, I2C_ADDR); // Function set: 8-bit mode (first attempt)
	HAL_Delay(5); // Wait >4.1ms
	CharLCD_Write_Nibble(hi2c1, 0x03, 0, false, I2C_ADDR); // Function set: 8-bit mode (second attempt)
	HAL_Delay(1); // Wait >100us
	CharLCD_Write_Nibble(hi2c1, 0x03, 0, false, I2C_ADDR); // Function set: 8-bit mode (third attempt)
	HAL_Delay(1); // Wait >100us
	CharLCD_Write_Nibble(hi2c1, 0x02, 0, false, I2C_ADDR); // Function set: switch to 4-bit mode
	CharLCD_Send_Cmd(hi2c1, 0x28, I2C_ADDR); // Function set: 4-bit, 2 lines, 5x8 font
	CharLCD_Send_Cmd(hi2c1, 0x0C, I2C_ADDR); // Display control: display on/cursor off/blink off
	CharLCD_Send_Cmd(hi2c1, 0x06, I2C_ADDR); // Entry mode: increment cursor, no shift
	CharLCD_Send_Cmd(hi2c1, 0x01, I2C_ADDR); // Clear display
	HAL_Delay(2); // Wait for clear display command
}

/**
 * @brief Clear LCD display and return cursor to home position
 * @param None
 * @retval None
 */
void CharLCD_Clear(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR) {
	CharLCD_Send_Cmd(hi2c1, 0x01, I2C_ADDR); // Clear display command
	HAL_Delay(2); // Wait for command execution
}

/**
 * @brief Send command to LCD
 * @param cmd: 8-bit command to send to LCD controller
 * @retval None
 */
void CharLCD_Send_Cmd(I2C_HandleTypeDef *hi2c1, uint8_t cmd, uint8_t I2C_ADDR) {
	uint8_t upper_nibble = cmd >> 4; // Extract upper 4 bits
	uint8_t lower_nibble = cmd & 0x0F; // Extract lower 4 bits
	CharLCD_Write_Nibble(hi2c1, upper_nibble, 0, false, I2C_ADDR); // Send upper nibble (DC=0 for command)
	CharLCD_Write_Nibble(hi2c1, lower_nibble, 0, false, I2C_ADDR); // Send lower nibble (DC=0 for command)
	if (cmd == 0x01 || cmd == 0x02) { // Clear display or return home commands
	HAL_Delay(2); // These commands need extra time
 }
}

/**
 * @brief Send data (character) to LCD
 * @param data: 8-bit character data to display
 * @retval None
 */
void CharLCD_Send_Data(I2C_HandleTypeDef *hi2c1, uint8_t data, bool motion_detected, uint8_t I2C_ADDR) {
	uint8_t upper_nibble = data >> 4; // Extract upper 4 bits
	uint8_t lower_nibble = data & 0x0F; // Extract lower 4 bits
	CharLCD_Write_Nibble(hi2c1, upper_nibble, 1, motion_detected, I2C_ADDR); // Send upper nibble (DC=1 for data)
	CharLCD_Write_Nibble(hi2c1, lower_nibble, 1, motion_detected, I2C_ADDR); // Send lower nibble (DC=1 for data)
}

/**
 * @brief Set cursor position on LCD
 * @param row: Row number (0 or 1 for 2-line display)
 * @param column: Column number (0 to display width - 1)
 * @retval None
 */
void CharLCD_Set_Cursor(I2C_HandleTypeDef *hi2c1, uint8_t row, uint8_t column, uint8_t I2C_ADDR) {
	uint8_t address;
	switch (row) {
		case 0:
			address = 0x00; break; // First line starts at address 0x00
		case 1:
			address = 0x40; break; // Second line starts at address 0x40
		default:
			address = 0x00; // Default to first line for invalid row
	}
	address += column; // Add column offset
	CharLCD_Send_Cmd(hi2c1, 0x80 | address, I2C_ADDR); // Set DDRAM address command (0x80 + address)
}

/**
 * @brief Write a 4-bit nibble to the LCD via I2C
 * @param nibble: 4-bit data to send (lower 4 bits)
 * @param dc: data/command (1 = data, 0 = command)
 * @retval None
 */
void CharLCD_Write_Nibble(I2C_HandleTypeDef *hi2c1, uint8_t nibble, uint8_t dc, bool motion_detected, uint8_t I2C_ADDR) {
	uint8_t data = nibble << D4_BIT; // Shift nibble to D4-D7 position
	data |= (dc & 0x01U) << DC_BIT; // Set DC bit for data/command selection
	if (lcd_backlight_enabled) {
		data |= (1U << BL_BIT);
	} else {
		data &= ~(1U << BL_BIT);
	}
	lcd_port_state = data & ~(1U << EN_BIT);
	data |= 1U << EN_BIT; // Set enable bit high
	CharLCD_Write_Port(hi2c1, I2C_ADDR, data); // Send data with EN high
	HAL_Delay(1); // Wait for data setup
	CharLCD_Write_Port(hi2c1, I2C_ADDR, lcd_port_state); // Send data with EN low
}

/**
 * @brief Write a string to the LCD at the current cursor position
 * @param hi2c1: Pointer to I2C handle (e.g., &hi2c1)
 * @param I2C_ADDR: I2C address of the LCD
 * @param str: Null-terminated string to display
 * @retval None
 */
void CharLCD_Write_String(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, char str[]){
	for (int i = 0; str[i] != '\0'; i++) {
		CharLCD_Send_Data(hi2c1, str[i], false, I2C_ADDR);
	}
}

/**
 * @brief Write a string to the LCD at the current cursor position
 * @param display: Pointer to a boolean indicating if the display is active
 * @param rotatingMessage: Buffer to hold the message to display
 * @param new_str: New string to display
 * @param MAX_LEN: Maximum length of the message buffer
 * @retval None
 */
void StartText(bool *display, char rotatingMessage[], const char *new_str, uint16_t MAX_LEN){
	*display = true;
	strncpy(rotatingMessage, new_str, MAX_LEN-1);
	//TODO: Add support for multiple strings? AFTER minimum ver release
}

/**
 * @brief Stop displaying text on the LCD and clear the display
 * @param display: Pointer to a boolean indicating if the display is active
 * @param hi2c1: Pointer to I2C handle (e.g., &hi2c1)
 * @param I2C_ADDR: I2C address of the LCD
 * @retval None
 */
void StopText(bool *display, I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR){
	*display = false;
	CharLCD_Clear(hi2c1, I2C_ADDR);
}

/**
 * @brief Write a frame to the screen
 * @param hi2c1: Pointer to I2C handle (e.g., &hi2c1)
 * @param I2C_ADDR: I2C address of the LCD
 * @param LCD_COLS: Number of columns on the LCD
 * @param scroll_pos: Current scroll position in the message
 * @param message: The message to display
 * @retval None
 */
void ShowFrame(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, uint16_t LCD_COLS, int scroll_pos, char message[]){
    static uint32_t pos = 0;
	uint32_t length = strlen(message);
	for(int i=0; i<LCD_COLS; i++){
		CharLCD_Set_Cursor(hi2c1, 0, i, I2C_ADDR);
		pos = (i+scroll_pos) % length;
		CharLCD_Send_Data(hi2c1, message[pos], false, I2C_ADDR);
	}
}

/**
 * @brief Turn on the LCD backlight
 * @param hi2c1: Pointer to I2C handle (e.g., &hi2c1)
 * @param I2C_ADDR: I2C address of the LCD
 * @retval None
 */
void LCD_Backlight_On(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR)
{
    lcd_backlight_enabled = true;
    lcd_port_state |= (1U << BL_BIT);
    CharLCD_Write_Port(hi2c1, I2C_ADDR, lcd_port_state);
}

/**
 * @brief Turn off the LCD backlight
 * @param hi2c1: Pointer to I2C handle (e.g., &hi2c1)
 * @param I2C_ADDR: I2C address of the LCD
 * @retval None
 */
void LCD_Backlight_Off(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR)
{
    lcd_backlight_enabled = false;
    lcd_port_state &= ~(1U << BL_BIT);
    CharLCD_Write_Port(hi2c1, I2C_ADDR, lcd_port_state);
}