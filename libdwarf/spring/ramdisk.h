#pragma once
#include "types.h"

#define RAMDISK_MAGIC "MINOSRAMDISK...."
#define RAMDISK_MAGIC_SIZE 16

#define RAMDISK_FNAME_SIZE 32

struct ramdisk_inode {
	char f_name[RAMDISK_FNAME_SIZE];
	uint64_t f_offset;	// data offset from ramdisk_start.
	uint64_t f_size;	// data size of this file
} __attribute__((__packed__));

struct ramdisk_sb {
	uint32_t file_cnt;
	uint32_t block_size;	// always 4096
	uint64_t inode_offset;	// inode offset
	uint64_t data_offset;	// file data offset.
	uint64_t ramdisk_size;	// total size of the ramdisk.
};

struct ramdisk_file {
	struct ramdisk_inode *inode;
	unsigned long pos;	// reserved
};

int ramdisk_read(struct ramdisk_file *file, void *buf, size_t size, unsigned long offset);
int ramdisk_open(char *name, struct ramdisk_file *file);
int init_ramdisk(void);