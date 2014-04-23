#ifndef __MAIN_H
#define __MAIN_H

#define DEFAULT_FILENAME "out.undeleted"

#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include <stdbool.h>

struct options {
	int state;
	ext2_ino_t ino;
	char * device;
	char * output_name;
        char * original_name;
        bool strip;
};

#endif