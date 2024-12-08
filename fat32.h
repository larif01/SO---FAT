#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#define DIR_FREE_ENTRY 0xE5

#define DIR_ATTR_READONLY 0x01 /* file is read only */
#define DIR_ATTR_HIDDEN 0x02 /* file is hidden */
#define DIR_ATTR_SYSTEM 0x04 /* system file (also hidden) */
#define DIR_ATTR_VOLUMEID 0x08 /* special entry containing disk volume label */
#define DIR_ATTR_DIRECTORY 0x10 /* describes a subdirectory */
#define DIR_ATTR_ARCHIVE 0x20 /* archive flag (always set when file is modified) */
#define DIR_ATTR_LFN 0x0F /* long file name attribute */

#define SIG 0xAA55 /* boot sector signature -- sector is executable */

#pragma pack(push, 1)
struct fat_dir {
    uint8_t name[11]; /* Short name + file extension */
    uint8_t attr; /* file attributes */
    uint8_t ntres; /* reserved for Windows NT, Set value to 0 when a file is created. */
    uint8_t creation_stamp; /* millisecond timestamp at file creation time */
    uint16_t creation_time; /* time file was created */
    uint16_t creation_date; /* date file was created */
    uint16_t last_access_date; /* last access date (last read/written) */
    uint16_t first_cluster_high; /* high 16 bits of the first cluster number */
    uint16_t last_write_time; /* time of last write */
    uint16_t last_write_date; /* date of last write */
    uint16_t starting_cluster; /* starting cluster (low 16 bits) */
    uint32_t file_size; /* 32-bit */
};

/* Boot Sector and BPB
 * Located at the first sector of the volume in the ⬤
