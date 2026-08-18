/**
 * @file app_comm.c
 * @brief App Communication Module
 * This file contains the implementation of the communication functions for the application.
 */

#include "app_comm.h"
#include "drv_eeprom24xx08.h"
#include "drv_i2clcd.h"
#include "string.h"
#include "stdbool.h"

#define MAX_STRING_LENGTH (128)
#define MAX_STRINGS (5)
#define MAX_MESSAGE_LENGTH ((MAX_STRINGS) * (MAX_STRING_LENGTH))

#define MESSAGE_BLOCK 0 // Block 0 of the EEPROM
#define MESSAGE_START_ADDRESS 0 // Start address of the message in EEPROM

char strings[MAX_STRINGS][MAX_STRING_LENGTH];
char printMessage[MAX_MESSAGE_LENGTH] = {0};
uint8_t current_pos = 0;
uint8_t scroll_pos = 0;
size_t current_offset = 0;

/**
 * @brief Add a string to the existing `strings` list
 * @param message The message to be added
 * @param size The length of the message to be added
 */
void add(char *str, size_t size) {
    if (size == 0) return;

    if (current_pos >= MAX_STRINGS) return;

    size_t copy_len = (size < (MAX_STRING_LENGTH - 1)) ? size : (MAX_STRING_LENGTH - 1);
    memcpy(strings[current_pos], str, copy_len);
    current_pos++;
}

/**
 * @brief Remove the last string in the list
 */
void rem(void){
    if (current_pos > 0) {
        --current_pos;
        memset(strings[current_pos], 0, sizeof(strings[current_pos]));
    }
    memset(printMessage, 0, sizeof(printMessage));
    scroll_pos = 0;
}

/**
 * @brief Remove all strings in the list
 */
void clr(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR){
    CharLCD_Clear(hi2c1, I2C_ADDR);
    memset(printMessage, 0, sizeof(printMessage));
    for (int i = 0; i < MAX_STRINGS; i++) {
        memset(strings[i], 0, sizeof(strings[i]));
    }
    current_pos = 0;
    scroll_pos = 0;
}

/**
 * @brief Build message with the current contents of `strings`. Allows `writeScrolling` to be executed immediately after
 */
void buildMessage(void) {
    // Clear the buffer
    memset(printMessage, 0, sizeof(printMessage));
    current_offset = 0;

    for (int i = 0; i < current_pos; i++) {
        size_t len = strlen(strings[i]);

        // Ensure we have enough space for the string + a null terminator
        if (current_offset + len + 1 <= sizeof(printMessage)) {
            
            // Copy the string data into the buffer at the current offset
            memcpy(&printMessage[current_offset], strings[i], len);
            current_offset += len;

            // Explicitly add the null terminator to separate the strings
            printMessage[current_offset] = '\0';
            current_offset++; 
            
        } else {
            // Buffer is full, stop adding strings
            break; 
        }
    }

    // Add the 0x03 (End of Text) marker if there is remaining room
    if (current_offset + 1 <= sizeof(printMessage)) {
        printMessage[current_offset] = 0x03;
        printMessage[current_offset + 1] = '\0'; 
    }
    
    scroll_pos = 0;
}

bool needsScroll(uint16_t LCD_COLS){
    return (current_offset > LCD_COLS);
}


/**
 * @brief Write a message without scrolling to the LCD. Expects message to have already been built with `buildMessage`
 */
void writeNoScroll(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR){
    CharLCD_WriteNoScroll(hi2c1, I2C_ADDR, printMessage);
}

/**
 * @brief Displays the `message` `char`
 */
void writeScrolling(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR, uint8_t LCD_COLS){
    CharLCD_WriteScrolling(hi2c1, I2C_ADDR, LCD_COLS, scroll_pos, printMessage);
    if(scroll_pos < current_offset - LCD_COLS){
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
HAL_StatusTypeDef readFromEEPROM(I2C_HandleTypeDef *hi2c1){
    HAL_StatusTypeDef eeprom_status;
    uint8_t address = MESSAGE_START_ADDRESS;
    char buffer[MAX_STRING_LENGTH]; // a real, correctly-sized buffer
    do {
        eeprom_status = EEPROM_ReadMessage(hi2c1, MESSAGE_BLOCK, address, buffer, sizeof(buffer));

        if (eeprom_status != HAL_OK) break;

        size_t len = strlen(buffer);
        if (len == 0) break; // empty string = hit the 0xFF padding = no more stored messages
        
        add(buffer, len);

        //check for 0x03 end of text control char
        if(strchr(buffer, 0x03) != NULL) break;
        
        address += (len + 1); // move past message and null terminator

    } while (address < 256); //stay within one block

    return eeprom_status;
}

/**
 * @brief Write `strings` array to EEPROM
 * @param hi2c1 Pointer to I2C handle (e.g., &hi2c1)
 * @return HAL_StatusTypeDef indicating success or failure
 */
HAL_StatusTypeDef writeToEEPROM(I2C_HandleTypeDef *hi2c1){
    HAL_StatusTypeDef eeprom_status = HAL_OK;

    eeprom_status = EEPROM_WritePage(
        hi2c1,
        MESSAGE_BLOCK,
        MESSAGE_START_ADDRESS,
        (const uint8_t *)printMessage,
        current_offset
    );

    return eeprom_status;
}