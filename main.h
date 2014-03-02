#ifndef __MAIN_H
#define __MAIN_H

#define DEFAULT_FILENAME "out.undeleted"

struct options {
	int state;
	int inode_offset;
	char * device;
	char * output_name;
};

#endif