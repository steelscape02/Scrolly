/**
app_comm.c
This file contains the implementation of the communication functions for the application.
*/

#include "app_comm.h"
#include "drv_eeprom24xx08.h"
#include "string.h"

#define MAX_STRING_LENGTH 128
#define MAX_STRINGS 5

char strings[MAX_STRINGS][MAX_STRING_LENGTH];
uint8_t current_pos = 0;

/**
 * @brief Add a string to the existing `strings` list
 * @param hi2c1 Pointer to I2C handle (e.g., &hi2c1)
 * @param message The message to be added
 * @param size The length of the message to be added
 */
void add(I2C_HandleTypeDef *hi2c1, char *message, size_t size) {

    if (size < MAX_STRING_LENGTH - 1){

        if (current_pos < MAX_STRINGS) {

            strncpy(strings[current_pos], message, MAX_STRING_LENGTH - 1);
            strings[current_pos][MAX_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
            current_pos++;
        }
    }
}

/**
 * @brief Remove the last string in the list
 * @param hi2c1 Pointer to I2C handle (e.g., &hi2c1)
 */
void rem(I2C_HandleTypeDef *hi2c1){

    if (current_pos > 0 && current_pos <= MAX_STRINGS) {
        --current_pos;
        memset(strings[current_pos], 0, sizeof(strings[current_pos]));
    }
}

/**
 * @brief Remove all strings in the list
 * @param hi2c1 Pointer to I2C handle (e.g., &hi2c1)
 */
void clr(I2C_HandleTypeDef *hi2c1){

    for (int i=0; i<MAX_STRINGS; i++){
        memset(strings[i], 0, sizeof(strings[i]));
    }
}