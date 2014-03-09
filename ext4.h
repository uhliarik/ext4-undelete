#ifndef _EXT4_H
#define _EXT4_H

#include <linux/types.h>

/* data type for filesystem-wide blocks number */
typedef unsigned long long ext4_fsblk_t;

/* data type for file logical block number */
typedef __u32 ext4_lblk_t;

#endif