/* 
 * File:   ext4_undelete.c
 * Author: Uhliarik Lubos
 *
 * Created on March 2, 2014, 9:57 PM
 */
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <asm/byteorder.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "ext4_undelete.h"
#include "debug.h"

static int ext4_ext_more_to_iterate(struct ext4_ext_path *path) {
    if (path->p_idx > EXT_LAST_INDEX(path->p_hdr))
        return 0;

    return 1;
}

static void block_release(struct block_buffer *bb) {
    if (bb != NULL) {
        if (bb->data != NULL) {
            free(bb->data);
            bb->data = NULL;
        }
        free(bb);
    }
}

static struct block_buffer *read_block(char *device, ext4_fsblk_t pblk){
    struct block_buffer *bb = NULL;
    int fd = 0;
    
    bb = malloc(sizeof(struct block_buffer));
    if(bb == NULL){
        fprintf(stderr, "Error: unable to allocate enough memory!\n");
        return NULL;
    }
    
    bb->data = malloc(EXT4_BLOCK_SIZE);
    if(bb->data == NULL){
        free(bb);
        fprintf(stderr, "Error: unable to allocate enough memory!\n");
        return NULL;
    }
    
    fd = open(device, O_LARGEFILE | O_RDONLY);
    if (fd < 0){
        block_release(bb);
        fprintf(stderr, "Error: unable to open input device!\n");
        return NULL;
    }
    
    off_t offset = pblk * EXT4_BLOCK_SIZE;
    if (lseek(fd, offset, SEEK_SET) < 0){
        close(fd);
        block_release(bb);
        fprintf(stderr, "Error: unable to seek in input device!\n");
        return NULL;
    }
    
    if(EXT4_BLOCK_SIZE != read(fd, bb->data, EXT4_BLOCK_SIZE)){
        close(fd);
        block_release(bb);
        fprintf(stderr, "Error: unable to read from input!\n");
        return NULL;
    }
    
    close(fd);
    
    return bb;
}


void print_ext4_exhdr_info(struct ext4_extent_header *header) {
    printf("******* Printing extent header info ********\n");
    printf("eh_magic: %04X\n", header->eh_magic);
    printf("eh_entries: %d\n", header->eh_entries);
    printf("eh_max: %d\n", header->eh_max);
    printf("eh_depth: %d\n", header->eh_depth);
    printf("eh_generation: %08X\n", (header->eh_generation));
}

void print_ext4_inode_info(struct ext4_inode * inode) {
    printf("******* Printing inode info ********\n");
    printf("mode: %d\n", inode->i_mode);
    printf("uid: %d\n", inode->i_uid);
    printf("size_lo: %d\n", inode->i_size_lo);
    printf("atime: %d\n", inode->i_atime);
    printf("ctime: %d\n", inode->i_ctime);
    printf("mtime: %d\n", inode->i_mtime);
    printf("dtime: %d\n", inode->i_dtime);
    printf("gid: %d\n", inode->i_gid);
    printf("links count: %d\n", inode->i_links_count);
    printf("blocks_lo: %d\n", inode->i_blocks_lo);
    printf("flags: %d\n", inode->i_flags);
    printf("size_high: %d\n", inode->i_size_high);
}

int ext4_read_inode(char *device, off_t offset, struct ext4_inode *inode) {
    D(printf("Opening file: %s\n", device));

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: file not found!\n");
        return -1;
    }

    D(printf("Seeking to position: %lld\n", offset));
    if (lseek(fd, offset, SEEK_SET) < 0) {
        fprintf(stderr, "Error: can NOT set offset to %lld\n", offset);
        close(fd);
        return -1;
    }

    int ret;
    if ((ret = read(fd, inode, sizeof (struct ext4_inode))) < 0) {
        fprintf(stderr, "Error: error during reading data from device\n");
        close(fd);
        return -1;
    }
    D(printf("Read %d bytes from file\n", ret));

    return fd;
}

static int append_undeleted_data(int fd, int out_fd, ext4_lblk_t ex_ee_block, 
                                 ext4_fsblk_t pblk, unsigned short ex_ee_len){
    
    D(printf("Entering append_undeleted_data function \n"));
    void * buffer;
    off_t offset = (off_t)pblk * EXT4_BLOCK_SIZE;
    if (lseek(fd, offset, SEEK_SET) < 0) {
        fprintf(stderr, "Error: can NOT set offset to %lld\n", offset);
        return -1;
    }
    
    off_t out_offset = (off_t)ex_ee_block * EXT4_BLOCK_SIZE; 
    if(lseek(out_fd, out_offset, SEEK_SET) < 0){
        fprintf(stderr, "Error: unable to seek to position: %lld", out_offset);
        return -1;
    }
    
    off_t bytes_to_transfer = ex_ee_len * EXT4_BLOCK_SIZE;
    buffer = (void *)malloc(EXT4_BLOCK_SIZE);
    if(buffer == NULL){
        fprintf(stderr, "Error: unable to allocate memory for buffer!");
        return -1;
    }
    
    int bytes_read = 0, bytes_written = 0;
    off_t write_off = 0;
    while(bytes_to_transfer){
        if((bytes_read = read(fd, (char *)buffer, EXT4_BLOCK_SIZE)) < 0){
            // ERROR
        }
        
        while(bytes_read){
                if((bytes_written = write(out_fd, ((char *)buffer)+write_off, bytes_read-write_off)) < 0){
                    // ERROR
                }
                bytes_read -= bytes_written;
                write_off += bytes_written;
        }
        
        bytes_to_transfer -= write_off;
        bytes_read = bytes_written = 0;
        write_off = 0;
    }
    
    return 0;
}

static int ext4_undelete_leaf(struct ext4_ext_path *path, int out_fd, int fd) {
    struct ext4_extent *ex;
    ext4_lblk_t ex_ee_block;
    ext4_fsblk_t pblk;
    unsigned short ex_ee_len;
    struct ext4_extent_header *eh;
    
    if (!path->p_hdr){
        path->p_hdr = ext_block_hdr(path->p_bb);
        path->p_hdr->eh_entries = __cpu_to_le16(ext4_ext_get_entries(path->p_hdr));
    }
    
    eh = path->p_hdr;
    print_ext4_exhdr_info(eh);
    D(printf("Restoring eh_entries from eh_generation: %d\n", __le16_to_cpu(path->p_hdr->eh_entries)));
    
    ex = EXT_FIRST_EXTENT(eh);
    ex_ee_block = __le32_to_cpu(ex->ee_block);
    ex_ee_len = ext4_ext_get_actual_len(ex);
    pblk = ext4_ext_pblock(ex);
    
    // process all leafs
    while (ex <= EXT_LAST_EXTENT(eh)) {
        D(printf("DATA: Extent starts at block: %lld (len: %d, first block's number: %d). Processing leaf node..\n",  pblk, ex_ee_len, ex_ee_block));
        
        append_undeleted_data(fd, out_fd, ex_ee_block, pblk, ex_ee_len);
        
        ex++;
        ex_ee_block = __le32_to_cpu(ex->ee_block);
        ex_ee_len = ext4_ext_get_actual_len(ex);
        pblk = ext4_ext_pblock(ex);
    }
    
    return 0;
}

static int open_out_file(char *output_name){
    int fd = open(output_name, O_CREAT | O_RDWR | O_LARGEFILE, 0600);
    
    return fd;
}

static void close_out_file(int out_fd){
    if(out_fd > 0)
       close(out_fd);
}

// TODO: set offset to bigger data type (should be at least 48 bytes long...)

int ext4_undelete_file(char *device, off_t offset, char *output_name) {
    struct ext4_inode inode;
    struct ext4_ext_path *path = NULL;
    int i = 0, err = 0;
    int depth = 0;
    int fd, out_fd;
    
    if ((fd = ext4_read_inode(device, offset, &inode)) < 0)
        return -1;

    print_ext4_inode_info(&inode);
    print_ext4_exhdr_info(ext_inode_hdr(&inode));

    // first, read inode by given offset
    // then start iterating the tree
    // try to reconstruct file...

    depth = ext4_ext_get_depth(ext_inode_hdr(&inode)); // get depth from eh_generation and check it !!!
    D(printf("Restoring eh_depth from eh_generation: %d\n", depth));
    if (depth < 0 || depth > EXT4_EXT_MAX_DEPTH) {
        fprintf(stderr, "Error: invalid eh_depth in inode!!!\n");
        return -1;
    }

    path = calloc(depth + 1, sizeof (struct ext4_ext_path));
    if (path == NULL) {
        fprintf(stderr, "Error: can't allocate enough memory!\n");
        return -1;
    }

    path[0].p_depth = depth;
    path[0].p_hdr = ext_inode_hdr(&inode);
    path[0].p_hdr->eh_entries = __cpu_to_le16(ext4_ext_get_entries(path[0].p_hdr));
    D(printf("Restoring eh_entries from eh_generation: %d\n", __le16_to_cpu(path[0].p_hdr->eh_entries)));

    out_fd = open_out_file(output_name);
    if (out_fd < 0){
        fprintf(stderr, "Error: can't write to output file: %s!\n", output_name);
        if (path != NULL)
                free(path);
        
        return -1;
    }
    
    i = 0;
    while (i >= 0) {
        // leaf block
        if (i == depth) {
            D(printf("Processing leaf node..\n"));

            err = ext4_undelete_leaf(path + i, out_fd, fd);
            if (err < 0){
                // ERROR
            }
            
            block_release(path[i].p_bb);
            path[i].p_bb = NULL;
            i--;

            continue;
        }

        // index block
        if (!path[i].p_hdr) {
            path[i].p_hdr = ext_block_hdr(path[i].p_bb);
        }

        if (!path[i].p_idx) {
            /* this level hasn't been touched yet */
            path[i].p_idx = EXT_FIRST_INDEX(path[i].p_hdr);
            path[i].p_block = ext4_ext_get_entries(path[i].p_hdr);
            path[i].p_hdr->eh_entries = __cpu_to_le16(ext4_ext_get_entries(path[i].p_hdr));
        } else {
            /* we were already here, see at next index */
            path[i].p_idx++;
        }

        if (ext4_ext_more_to_iterate(path + i)) {
            struct block_buffer *bb = NULL;
            D(printf("Reading block: %lld, index: %d\n", ext4_idx_pblock(path[i].p_idx), i));
            
            /* go to the next level */
            memset(path + i + 1, 0, sizeof (*path));
            
            bb = read_block(device, ext4_idx_pblock(path[i].p_idx));
            if(bb == NULL){
                // TODO: ERROR
            }
            
            if (i + 1 > depth) {
                // TODO: ERROR
            }
            path[i + 1].p_bb = bb;

            i++;
        } else {
            /* root level has p_bb == NULL, block_release() eats this */
            block_release(path[i].p_bb);
            path[i].p_bb = NULL;
            i--;
        }
    }

    if (path != NULL)
        free(path);
    
    D(printf("Closing files...\n"));
    
    close(fd);
    close_out_file(out_fd);
    
    return 0;
}
