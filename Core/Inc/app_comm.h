#ifndef INC_APP_COMM_H_
#define INC_APP_COMM_H_

#include "main.h"
#include "stdbool.h"
#include "string.h"

bool add(char *message, size_t len);
void rem(void);
void clr(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
bool needsScroll(uint16_t LCD_COLS);
void writeNoScroll(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
void writeScrolling(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, uint8_t LCD_COLS);
HAL_StatusTypeDef EEPROM_ReadBuffer(I2C_HandleTypeDef *hi2c1);
HAL_StatusTypeDef writeToEEPROM(I2C_HandleTypeDef *hi2c1);
HAL_StatusTypeDef EEPROM_WriteBuffer(I2C_HandleTypeDef *hi2c, uint8_t block, uint16_t mem_addr, const uint8_t *pData, uint16_t size);

#endif /* INC_APP_COMM_H_ */