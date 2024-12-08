#ifndef OUTPUT_H
#define OUTPUT_H

#include "fat32.h"  // Alterado para incluir o cabeçalho FAT32

void show_files(struct fat_dir *);

void verbose(struct fat_bpb *);

#endif
