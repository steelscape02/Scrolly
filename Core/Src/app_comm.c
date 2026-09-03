/**
 * @file app_comm.c
 * @brief App Communication Module
 * This file contains the implementation of the communication functions for the application.
 */

#include "app_comm.h"
#include "drv_eeprom24xx08.h"
#include "drv_i2clcd.h"
#include "stm32l4xx_hal_uart.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"

#define MAX_STRING_LENGTH (128)
#define MAX_STRINGS (5)
#define MAX_MESSAGE_LENGTH ((MAX_STRINGS) * (MAX_STRING_LENGTH))
#define DEBUG_MESSAGE_LENGTH (64)

#define MESSAGE_BLOCK 0 // Block 0 of the EEPROM
#define HEADER_ADDRESS 0x00
#define MESSAGE_START_ADDRESS 0 // Start address of the message in EEPROM

typedef struct __attribute__((packed)) {
    uint8_t count;
    uint16_t tail;
    uint16_t offsets[MAX_STRINGS];
} EEPROM_Header_t;

char strings[MAX_STRINGS][MAX_STRING_LENGTH];
char printMessage[MAX_MESSAGE_LENGTH] = {0};
uint8_t current_pos = 0;
uint8_t scroll_pos = 0;
size_t current_offset = 0;

// direct memory offset metadata and pool
char message_pool[MAX_MESSAGE_LENGTH]; // Raw string data: "Hello\0World\0"
uint16_t string_offsets[MAX_STRINGS]; // Stores start index of each string
uint8_t string_count = 0;
uint16_t pool_tail = 0;

// -- DEBUG -- //
static UART_HandleTypeDef *log_uart = NULL;
static char debug_msg[DEBUG_MESSAGE_LENGTH];

void sensorInit(UART_HandleTypeDef *huart) {
    log_uart = huart;
}

void createMetadataString(char *buf, size_t buf_size, uint8_t string_count, uint8_t pool_tail){
    snprintf(buf, buf_size, "string_count: %u\r\npool_tail: %u\r\n", string_count, pool_tail);
}

/**
 * @brief Add a string to the existing `strings` list
 * @param message The message to be added
 * @param size The length of the message to be added
 */
bool add(char *str, size_t len) {

    // 1. Check if we have room in the pool and index array
    // (+1 for the null terminator '\0')
    if ((pool_tail + len + 1 > MAX_MESSAGE_LENGTH) || (string_count >= MAX_STRINGS)) {
        return false; // Out of memory/capacity
    }

    string_offsets[string_count] = pool_tail;

    strcpy(&message_pool[pool_tail], str);

    pool_tail += (len + 1); // accomodate null terminator
    string_count++;

    return true;
}

/**
 * @brief Remove the last string in the list
 */
void rem(void){
    string_count--; // decrement strings count
    pool_tail = string_offsets[string_count];
}

/**
 * @brief Remove all strings in the list
 */
void clr(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR){
    CharLCD_Clear(hi2c1, I2C_ADDR);

    string_count = 0;
    pool_tail = 0;
}

bool needsScroll(uint16_t LCD_COLS){
    return (pool_tail > LCD_COLS);
}


/**
 * @brief Write a message without scrolling to the LCD. Expects message to have already been built with `buildMessage`
 */
void writeNoScroll(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR){
    //TODO: #26 Do the formatting here for adding spaces (may want to make a seperate method used for this and writeScrolling)
    CharLCD_WriteNoScroll(hi2c1, I2C_ADDR, message_pool, pool_tail);
}

/**
 * @brief Displays the `message` `char`
 */
void writeScrolling(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, uint8_t LCD_COLS){
    CharLCD_WriteScrolling(hi2c1, I2C_ADDR, LCD_COLS, scroll_pos, message_pool, pool_tail);
    if(scroll_pos < pool_tail - LCD_COLS){
        scroll_pos++;
    }else{
        scroll_pos = 0;
    }
}

/**
 * @brief Read EEPROM contents and split into `strings` array
 * @param hi2c1 Pointer to I2C handle (e.g., &hi2c1)
 * @return HAL_StatusTypeDef indicating success or failure
 */
HAL_StatusTypeDef EEPROM_ReadBuffer(I2C_HandleTypeDef *hi2c1) {
    HAL_StatusTypeDef status = HAL_OK;
    EEPROM_Header_t header;

    // 1. Read the metadata header from address 0x00
    status = EEPROM_ReadMessage(
        hi2c1,
        MESSAGE_BLOCK,
        HEADER_ADDRESS,
        (uint8_t *)&header,
        sizeof(EEPROM_Header_t)
    );
    if (status != HAL_OK) return status;

    // 2. Sanity check read data to prevent buffer overruns or corrupted EEPROM loads
    if (header.count > MAX_STRINGS || header.tail > MAX_MESSAGE_LENGTH) {
        // EEPROM holds uninitialized or corrupted data—reset local state
        string_count = 0;
        pool_tail = 0;
        return HAL_ERROR;
    }

    // 3. Restore control state and offsets
    string_count = header.count;
    pool_tail = header.tail;
    memcpy(string_offsets, header.offsets, sizeof(string_offsets));
    
    // debug message to show string_count and pool_tail on read
    createMetadataString(debug_msg, sizeof(debug_msg), string_count, pool_tail);
    HAL_UART_Transmit(log_uart, (uint8_t*)debug_msg, strlen(debug_msg), HAL_MAX_DELAY);


    // 4. Read the raw message_pool data back into RAM
    if (pool_tail > 0) {
        uint16_t pool_address = HEADER_ADDRESS + sizeof(EEPROM_Header_t);

        status = EEPROM_ReadMessage(
            hi2c1,
            MESSAGE_BLOCK,
            pool_address,
            (uint8_t *)message_pool,
            pool_tail
        );
    }else{
        snprintf(debug_msg, sizeof(debug_msg), "pool tail is 0\r\n");
        HAL_UART_Transmit(log_uart, (uint8_t*)debug_msg, strlen(debug_msg), HAL_MAX_DELAY);
    }

    return status;
}



HAL_StatusTypeDef writeToEEPROM(I2C_HandleTypeDef *hi2c1) {
    HAL_StatusTypeDef status = HAL_OK;

    // 1. Pack the metadata header
    EEPROM_Header_t header;
    header.count = string_count;
    header.tail = pool_tail;
    memcpy(header.offsets, string_offsets, sizeof(string_offsets));

    // 2. Write the metadata header to the start of the EEPROM
    status = EEPROM_WriteBuffer(
        hi2c1,
        MESSAGE_BLOCK,
        HEADER_ADDRESS,
        (const uint8_t *)&header,
        sizeof(EEPROM_Header_t)
    );
    if (status != HAL_OK) return status;

    // 3. Write raw message_pool data immediately following the header
    uint16_t pool_address = HEADER_ADDRESS + sizeof(EEPROM_Header_t);
    
    if (pool_tail > 0) {
        status = EEPROM_WriteBuffer(
            hi2c1,
            MESSAGE_BLOCK,
            pool_address,
            (const uint8_t *)message_pool,
            pool_tail
        );
    }

    return status;
}

/**
 * @brief Write `strings` array to EEPROM
 * @param hi2c1 Pointer to I2C handle (e.g., &hi2c1)
 * @return HAL_StatusTypeDef indicating success or failure
 */
// Helper to safely write arbitrary length data across 24LC08B page boundaries
HAL_StatusTypeDef EEPROM_WriteBuffer(I2C_HandleTypeDef *hi2c, uint8_t block, uint16_t mem_addr, const uint8_t *pData, uint16_t size) {
    HAL_StatusTypeDef status = HAL_OK;
    uint16_t bytes_written = 0;

    while (bytes_written < size) {
        // 24LC08B page size is 16 bytes
        uint8_t page_offset = (mem_addr + bytes_written) % 16;
        uint16_t chunk_size = 16 - page_offset;

        if (chunk_size > (size - bytes_written)) {
            chunk_size = size - bytes_written;
        }

        status = EEPROM_WritePage(hi2c, block, mem_addr + bytes_written, &pData[bytes_written], chunk_size);
        if (status != HAL_OK) return status;

        bytes_written += chunk_size;
        
        // 24LC08B requires up to 5ms tWR write cycle time between page writes
        HAL_Delay(5); 
    }

    return HAL_OK;
}