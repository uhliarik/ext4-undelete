/* 
 * File:   ext4_undelete.h
 * Author: Uhliarik Lubos
 *
 * Created on March 2, 2014, 9:57 PM
 */

#ifndef EXT4_UNDELETE_H
#define	EXT4_UNDELETE_H

#include "ext4.h"
#include "ext4_extents.h"

int ext4_undelete_file(char *device, off_t offset, char *output_name);
void print_ext4_exhdr_info(struct ext4_extent_header *header);
void print_ext4_inode_info(struct ext4_inode * inode);

#endif	/* EXT4_UNDELETE_H */

