#ifndef INC_APP_COMM_H_
#define INC_APP_COMM_H_

#include "main.h"
#include "stdbool.h"
#include "string.h"

void add(I2C_HandleTypeDef *hi2c1, char *message, size_t size);
void rem(I2C_HandleTypeDef *hi2c1);
void clr(I2C_HandleTypeDef *hi2c1);

#endif /* INC_APP_COMM_H_ */