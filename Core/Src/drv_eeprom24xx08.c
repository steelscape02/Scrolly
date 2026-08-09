/*
 * drv_eeprom24xx08.c
 * Support for 24LC08B EEPROM
 *
 *  Created on: Jul 18, 2026
 *      Author: nicho
 */
#include "drv_eeprom24xx08.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include <stdint.h>

#define EEPROM_I2C_ADDRESS 0x50 // Base 7-bit address for 24LC08B
#define EEPROM_PAGE_SIZE 16 // Page size in bytes for 24LC08B
#define EEPROM_TOTAL_SIZE 1024 // Total size in bytes for 24LC08B (8Kbit = 1024 bytes)
#define EEPROM_BLOCK_SIZE 256 // Size of each block in bytes (4 blocks of 256 bytes each)

/**
 * @brief Writes a single byte to the 24LC08B EEPROM.
 * @param hi2c1: Pointer to I2C handle (e.g., &hi2c1)
 * @param block: Block index (0 to 3)
 * @param word_address: Internal address within the block (0 to 255)
 * @param data: Byte of data to write
 * @return HAL_StatusTypeDef indicating success or failure
 */
HAL_StatusTypeDef EEPROM_WriteByte(I2C_HandleTypeDef *hi2c1, uint8_t block, uint8_t word_address, uint8_t data) {
    // Base 7-bit address (0x50) shifted left, with block bits packed into bits 1 and 2
    uint16_t dev_address = (EEPROM_I2C_ADDRESS | (block & 0x03)) << 1;

    uint8_t payload[2];
    payload[0] = word_address; // First byte:  Internal memory address
    payload[1] = data;         // Second byte: Data to store


    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c1, dev_address, payload, 2, 100);
    HAL_Delay(5); // Write cycle delay
    return status;
}

/**
  * @brief  Writes data chunks to the 24LC08B EEPROM safely across pages and blocks.
  * @param  hi2c: Pointer to I2C handle (e.g., &hi2c1)
  * @param  block: Starting block index (0 to 3)
  * @param  start_address: Internal address within the starting block (0 to 255)
  * @param  data: Pointer to the buffer containing data to write
  * @param  len: Total number of bytes to write
  */
HAL_StatusTypeDef EEPROM_WritePage(I2C_HandleTypeDef *hi2c, uint8_t block, uint8_t start_address, const uint8_t *data, size_t len) {
    size_t bytes_written = 0;

    // Convert starting block and offset into a single linear global address (0 to 1023)
    uint16_t global_address = ((uint16_t)(block & 0x03) << 8) | start_address;
    HAL_StatusTypeDef status = HAL_OK;

    // Make sure the parameters are valid and within EEPROM bounds
    if (hi2c == NULL || data == NULL || global_address >= EEPROM_TOTAL_SIZE || len > EEPROM_TOTAL_SIZE - global_address) {
        return HAL_ERROR;
    }

    // Write data in chunks, until we reach the string length or the end of EEPROM memory
    while (bytes_written < len) {
        uint16_t current_addr = global_address + bytes_written; // Calculate the current global address
        uint8_t current_block = (current_addr >> 8) & 0x03; // Flop over to next block if necessary
        uint8_t reg_addr = current_addr & 0xFF; // Find the offset in the current block (0-256)

        uint16_t dev_address = (EEPROM_I2C_ADDRESS | current_block) << 1; // Convert to 8-bit address for HAL functions

        // Calculate how many bytes can be written before hitting the page boundary
        size_t bytes_to_page_end = EEPROM_PAGE_SIZE - (reg_addr % EEPROM_PAGE_SIZE); 

        size_t bytes_to_write = len - bytes_written;
        if (bytes_to_write > bytes_to_page_end) {
            bytes_to_write = bytes_to_page_end;
        }

        status = HAL_I2C_Mem_Write(
            hi2c,
            dev_address,
            reg_addr,
            I2C_MEMADD_SIZE_8BIT,
            (uint8_t *)&data[bytes_written], // Dereference, get chunk, and return to pointer
            (uint16_t)bytes_to_write,
            HAL_MAX_DELAY
        );
        HAL_Delay(5); // Write cycle delay

        if (status == HAL_OK) {
            // Poll device readiness until it responds with an ACK
            status = HAL_I2C_IsDeviceReady(hi2c, dev_address, 10, HAL_MAX_DELAY);
            if (status != HAL_OK) {
                break;
            }
        } else {
            // Error handling (e.g., bus error, disconnected EEPROM)
            break;
        }

        bytes_written += bytes_to_write;
    }

    return status;
}

/**
 * @brief Erases a block in the EEPROM by writing 0xFF to all bytes in that block.
 * @param hi2c: Pointer to I2C handle (e.g., &hi2c1)
 * @param block: Block index to erase (0 to 3)
 * @return HAL_StatusTypeDef indicating success or failure
 */
HAL_StatusTypeDef EEPROM_EraseBlock(I2C_HandleTypeDef *hi2c, uint8_t block) {
    uint8_t erase_data[EEPROM_BLOCK_SIZE];
    memset(erase_data, 0xFF, sizeof(erase_data)); // Fill with 0xFF

    return EEPROM_WritePage(hi2c, block, 0, erase_data, EEPROM_BLOCK_SIZE);
}

/**
 * @brief Reads a sequence of bytes from the EEPROM.
 * @param hi2c: Pointer to I2C handle (e.g., &hi2c1)
 * @param block: Block index to read from (0 to 3)
 * @param reg_addr: Starting address within the block (0 to 255)
 * @param buffer: Pointer to buffer where read data will be stored
 * @param size: Number of bytes to read
 * @return HAL_StatusTypeDef indicating success or failure
 */
HAL_StatusTypeDef EEPROM_ReadSequential(I2C_HandleTypeDef *hi2c, uint8_t block, uint8_t reg_addr, uint8_t *buffer, uint16_t size){

	// Convert starting block and offset into a single linear global address (0 to 1023)
	uint16_t dev_address = (EEPROM_I2C_ADDRESS | (block & 0x03)) << 1;

	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
	    hi2c,
	    dev_address,
	    reg_addr,
		I2C_MEMADD_SIZE_8BIT,
	    buffer,
	    size,
	    HAL_MAX_DELAY
	);
    HAL_Delay(5); // Read cycle delay

	if (status == HAL_OK) {
		// Poll device readiness until it responds with an ACK
		HAL_I2C_IsDeviceReady(hi2c, dev_address, 10, HAL_MAX_DELAY);
	}else{
		//TODO: Handle error
	}
    return status;
}

/**
 * @brief Reads a null-terminated message from the EEPROM.
 * @param hi2c: I2C handle
 * @param block: EEPROM block number (0-3)
 * @param start_address: Starting address within the block
 * @param buffer: Buffer to store the read message
 * @param buffer_size: Size of the buffer
 * @return HAL_StatusTypeDef indicating success or failure
 */
HAL_StatusTypeDef EEPROM_ReadMessage(I2C_HandleTypeDef *hi2c, uint8_t block, uint8_t start_address, char *buffer, size_t buffer_size){

    size_t i = 0;
    uint16_t global_address = ((uint16_t)(block & 0x03) << 8) | start_address;
    HAL_StatusTypeDef status = HAL_OK;
    if (hi2c == NULL || buffer == NULL || buffer_size == 0 || block > 3) {
        return HAL_ERROR;
    }
    memset(buffer, 0, buffer_size);

    for (i = 0; i < buffer_size - 1 && global_address + i < EEPROM_TOTAL_SIZE; i++) {
		uint16_t current_addr = global_address + i;
		uint8_t current_block = (current_addr >> 8) & 0x03;
		uint8_t reg_addr = current_addr & 0xFF;
		uint16_t dev_address = (EEPROM_I2C_ADDRESS | current_block) << 1;

		// Read 1 byte at a time
        status = HAL_I2C_Mem_Read(
			hi2c,
			dev_address,
			reg_addr,
			I2C_MEMADD_SIZE_8BIT,
			(uint8_t*)&buffer[i],
			1,                     // 1 Byte
			HAL_MAX_DELAY
		);
        HAL_Delay(5); // Read cycle delay

        if (status != HAL_OK || buffer[i] == '\0' || (uint8_t)buffer[i] == 0xFF) {
			break;
		}
	}
	buffer[i] = '\0'; // Guarantee null-termination

    return status;
}

