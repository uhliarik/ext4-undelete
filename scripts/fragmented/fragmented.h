#ifndef __FRAGMENTED_H
#define __FRAGMENTED_H

#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include <stdbool.h>

#define DEFAULT_BLOCKSIZE 4096
#define DEFAULT_EXTENT_COUNT 5
#define DEFAULT_DATA_VAL 5

struct options {
	int state;
	unsigned int bs;
	unsigned long int extent_count;
        char *filename;
	unsigned long int data_val;
};

#endif