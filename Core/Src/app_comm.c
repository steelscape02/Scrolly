/**
app_comm.c
This file contains the implementation of the communication functions for the application.
*/

#include "app_comm.h"
#include "string.h"

#define MAX_STRING_LENGTH 128
#define MAX_STRINGS 5

char strings[MAX_STRINGS][MAX_STRING_LENGTH];
uint8_t current_pos = 0;

void add(char *message, size_t size) {

    if (size < MAX_STRING_LENGTH - 1){

        if (current_pos < MAX_STRINGS) {

            strncpy(strings[current_pos], message, MAX_STRING_LENGTH - 1);
            strings[current_pos][MAX_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
            current_pos++;
        }
    }
}