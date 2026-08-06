#ifndef INC_APP_COMM_H_
#define INC_APP_COMM_H_

#include "main.h"
#include "stdbool.h"
#include "string.h"

void add(char *message, size_t size);
void rem(void);
void clr(void);
void buildMessage(void);
bool needsScroll(uint16_t LCD_COLS);
void writeNoScroll(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
void writeScrolling(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, uint8_t LCD_COLS);
HAL_StatusTypeDef readFromEEPROM(I2C_HandleTypeDef *hi2c1);
HAL_StatusTypeDef writeToEEPROM(I2C_HandleTypeDef *hi2c1);

#endif /* INC_APP_COMM_H_ */