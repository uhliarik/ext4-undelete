#ifndef __MAIN_H
#define __MAIN_H


#define DEFAULT_FILENAME "out.undeleted"

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <sys/types.h>

struct options {
	int state;
	off_t inode_offset;
	char * device;
	char * output_name;
};

#endif