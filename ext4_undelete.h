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

/** using ext2fs lib */
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>

int undelete_file(char *device, ext2_ino_t ino, char *output_name);
void print_ext2_exhdr_info(struct ext3_extent_header *header);
void print_ext2_inode(struct ext2_inode * inode);

#endif	/* EXT4_UNDELETE_H */

