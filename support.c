#include "support.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "fat32.h"  // Alterado para incluir o cabeçalho FAT32

/* Manipulate the path to lead with name, extensions and special characters */
bool cstr_to_fat32wnull(char *filename, char output[FAT32STR_SIZE_WNULL])
{
    // Clear the output buffer
    memset(output, 0, FAT32STR_SIZE_WNULL);

    // Split the filename into name and extension
    char *dot = strrchr(filename, '.');
    size_t name_len = dot ? (size_t)(dot - filename) : strlen(filename);
    size_t ext_len = dot ? strlen(dot + 1) : 0;

    // Copy the name part
    for (size_t i = 0; i < name_len && i < 8; i++) {
        output[i] = toupper(filename[i]);
    }

    // Pad with spaces if the name is less than 8 characters
    for (size_t i = name_len; i < 8; i++) {
        output[i] = ' ';
    }

    // Copy the extension part
    if (dot) {
        for (size_t i = 0; i < ext_len && i < 3; i++) {
            output[8 + i] = toupper(dot[1 + i]);
        }
    }

    // Pad with spaces if the extension is less than 3 characters
    for (size_t i = ext_len; i < 3; i++) {
        output[8 + i] = ' ';
    }

    // Null-terminate the output
    output[11] = '\0';

    return false;
}
