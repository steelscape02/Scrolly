/**
app_comm.c
This file contains the implementation of the communication functions for the application.
*/

#include "app_comm.h"
#include "drv_eeprom24xx08.h"
#include "drv_i2clcd.h"
#include "string.h"

#define MAX_STRING_LENGTH (128)
#define MAX_STRINGS (5)
#define MAX_MESSAGE_LENGTH ((MAX_STRINGS) * (MAX_STRING_LENGTH))

#define MESSAGE_BLOCK 0 // Block 0 of the EEPROM
#define MESSAGE_START_ADDRESS 0 // Start address of the message in EEPROM

uint8_t strings[MAX_STRINGS][MAX_STRING_LENGTH];
uint8_t message[MAX_MESSAGE_LENGTH] = {0};
uint8_t current_pos = 0;
uint8_t scroll_pos = 0;

/**
 * @brief Add a string to the existing `strings` list
 * @param message The message to be added
 * @param size The length of the message to be added
 */
void add(char *message, size_t size) {

    if (size < MAX_STRING_LENGTH - 1){

        if (current_pos < MAX_STRINGS) {

            strncpy((char*)strings[current_pos], message, MAX_STRING_LENGTH - 1);
            strings[current_pos][MAX_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
            current_pos++;
        }
    }
}

/**
 * @brief Remove the last string in the list
 */
void rem(void){

    if (current_pos > 0 && current_pos <= MAX_STRINGS) {
        --current_pos;
        memset(strings[current_pos], 0, sizeof(strings[current_pos]));
    }
}

/**
 * @brief Remove all strings in the list
 */
void clr(void){

    for (int i=MAX_STRINGS; i>0; i--){
        memset(strings[i], 0, sizeof(strings[i]));
        current_pos = i;
    }
}

/**
 * @brief Build message with the current contents of `strings`. Allows `writeScrolling` to be executed immediately after
 */
void buildMessage(char message){
    // TODO: #7 Iterate through strings array and build a message string, then write to I2C with WriteScrolling
}

/**
 * @brief Write a message without scrolling to the LCD. Expects message to have already been built with `buildMessage`
 */
void writeNoScroll(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR){
    CharLCD_WriteNoScroll(hi2c1, I2C_ADDR, message);
}

/**
 * @brief Displays the `message` `char`
 */
void writeScrolling(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR){
    CharLCD_WriteScrolling(hi2c1, I2C_ADDR, scroll_pos, message);
    if(scroll_pos < strlen(rotatingMessage)){
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
    // Call EEPROM_ReadMessage repeatedly to get back new messages
    uint8_t start_address = MESSAGE_START_ADDRESS;
    char *buffer = NULL;
    size_t size;
    do{
        size = sizeof(buffer);
        eeprom_status = EEPROM_ReadMessage(hi2c1, MESSAGE_BLOCK, start_address, buffer, size);

        if(eeprom_status == HAL_OK) add(buffer, size);
        else break;
        
    }while(size > 0);

    return eeprom_status;
}

/**
 * @brief Write `strings` array to EEPROM
 * @param hi2c1 Pointer to I2C handle (e.g., &hi2c1)
 * @return HAL_StatusTypeDef indicating success or failure
 */
HAL_StatusTypeDef writeToEEPROM(I2C_HandleTypeDef *hi2c1){
    HAL_StatusTypeDef eeprom_status;

    for(int i=0; i<MAX_STRINGS; i++){
        uint8_t *str = strings[i];
        if(strlen((char*)str) > 0){
            eeprom_status = EEPROM_WritePage(
                hi2c1,
                MESSAGE_BLOCK,
                MESSAGE_START_ADDRESS,
                str,
                strlen((char*)str) + 1
            );

            if(eeprom_status != HAL_OK){
                break;
            }
        }
    }
    //TODO: Handle extra chars in EEPROM that weren't overwritten

    return eeprom_status;
}