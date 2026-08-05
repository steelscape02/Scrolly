#ifndef INC_DRV_I2CLCD_H_
#define INC_DRV_I2CLCD_H_

#include "main.h"
#include "stdbool.h"
#include "string.h"

// TODO: Standardize parameter placement
void CharLCD_Init(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
void CharLCD_Clear(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
void CharLCD_Send_Cmd(I2C_HandleTypeDef *hi2c1, uint8_t cmd, uint8_t I2C_ADDR);
void CharLCD_Send_Data(I2C_HandleTypeDef *hi2c1, uint8_t data, bool motion_detected, uint8_t I2C_ADDR);
void CharLCD_Set_Cursor(I2C_HandleTypeDef *hi2c1, uint8_t row, uint8_t column, uint8_t I2C_ADDR);
void CharLCD_Write_Nibble(I2C_HandleTypeDef *hi2c1, uint8_t nibble, uint8_t dc, bool motion_detected, uint8_t I2C_ADDR);
void CharLCD_Write_String(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, char str[]);
void CharLCD_WriteNoScroll(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, char message);
void CharLCD_WriteScrolling(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, char message);
void StartText(bool *display, char rotatingMessage[], const char *new_str, uint16_t MAX_LEN);
void StopText(bool *display, I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
void ShowFrame(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, uint16_t LCD_COLS, int scroll_pos, char message[]);
void LCD_Backlight_On(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
void LCD_Backlight_Off(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);

#endif /* INC_DRV_I2CLCD_H_ */
