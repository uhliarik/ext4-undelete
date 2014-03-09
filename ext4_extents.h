/*
 * Copyright (c) 2003-2006, Cluster File Systems, Inc, info@clusterfs.com
 * Written by Alex Tomas <alex@clusterfs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licens
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-
 */

#ifndef _EXT4_EXTENTS
#define _EXT4_EXTENTS

/** using ext2fs lib */
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include <ext2fs/ext3_extents.h>


//#include "ext4.h"
//#include <linux/byteorder/little_endian.h>

#define EXT4_EXT_MAX_DEPTH 5

struct block_buffer {
        void    *data;
};

/*
 * Array of ext4_ext_path contains path to some extent.
 * Creation/lookup routines use it for traversal/splitting/etc.
 * Truncate uses it to simulate recursive walking.
 */
struct ext_path {
	__u64   			p_block;
	__u16				p_depth;
	struct ext3_extent		*p_ext;
	struct ext3_extent_idx		*p_idx;
	struct ext3_extent_header	*p_hdr;
	struct block_buffer		*p_bb;
};

static inline int ext4_ext_get_actual_len(struct ext3_extent *ext)
{
	return (ext->ee_len <= EXT_INIT_MAX_LEN ?
		ext->ee_len :
		(ext->ee_len - EXT_INIT_MAX_LEN));
}

/*
 * ext4_ext_pblock:
 * combine low and high parts of physical block number into ext4_fsblk_t
 */
static inline ext4_fsblk_t ext4_ext_pblock(struct ext3_extent *ex)
{
	ext4_fsblk_t block;

	block = ex->ee_start;
	block |= ((ext4_fsblk_t) ex->ee_start_hi) << 31 << 1;
	return block;
}


static inline struct ext3_extent_header *ext_block_hdr(struct block_buffer *bb)
{
         return (struct ext3_extent_header *) bb->data;
}

/*
 * ext_idx_pblock:
 * combine low and high parts of a leaf physical block number into ext4_fsblk_t
 */
static inline ext4_fsblk_t ext_idx_pblock(struct ext3_extent_idx *ix)
{
	ext4_fsblk_t block;

	block = ix->ei_leaf;
	block |= ((ext4_fsblk_t) ix->ei_leaf_hi << 31) << 1;
	return block;
}

                                                                 
static inline struct ext3_extent_header *ext_inode_hdr(struct ext2_inode *inode)
{
	return (struct ext3_extent_header *) inode->i_block;
}

static inline __u16 ext4_ext_get_entries(struct ext3_extent_header *eh){
        __u16 eh_entries = 0x0;
        eh_entries |= eh->eh_generation >> 16;
        
        return eh_entries;
}

static inline __u16 ext4_ext_get_depth(struct ext3_extent_header *eh){
        __u16 eh_depth = 0x0;
        eh_depth |= eh->eh_generation;
        
        return eh_depth;
}


#endif /* _EXT4_EXTENTS */

