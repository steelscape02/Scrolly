#ifndef INC_APP_COMM_H_
#define INC_APP_COMM_H_

#include "main.h"
#include "stdbool.h"
#include "string.h"

void add(char *message, size_t size);
void rem(void);
void clr(void);
HAL_StatusTypeDef readFromEEPROM(I2C_HandleTypeDef *hi2c1);
HAL_StatusTypeDef writeToEEPROM(I2C_HandleTypeDef *hi2c1);

#endif /* INC_APP_COMM_H_ */