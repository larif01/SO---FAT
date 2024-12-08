#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdbool.h>
#include "fat32.h"  // Alterado para incluir o cabeçalho FAT32

bool cstr_to_fat32wnull(char *filename, char output[FAT32STR_SIZE_WNULL]);  // Alterado para FAT32

#endif
