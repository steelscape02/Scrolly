/*
 * eeprom.h
 *
 *  Created on: Jul 18, 2026
 *      Author: nicho
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "main.h"

void EEPROM_WriteByte(I2C_HandleTypeDef *hi2c1, uint8_t block, uint8_t word_address, uint8_t data);
void EEPROM_WritePage(I2C_HandleTypeDef *hi2c1, uint8_t block, uint8_t start_address, const uint8_t *data, size_t len);
void EEPROM_ReadRandom();
void EEPROM_ReadSequential(I2C_HandleTypeDef *hi2c, uint8_t block, uint8_t reg_addr, uint8_t *buffer, uint16_t size);
void EEPROM_ReadMessage(I2C_HandleTypeDef *hi2c, uint8_t block, uint8_t start_address, char *buffer);

#endif /* INC_EEPROM_H_ */
