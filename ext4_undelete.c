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
#include <libgen.h>
#include "ext4_undelete.h"
#include "debug.h"
#include "ext4_extents.h"

#include <ext2fs/ext3_extents.h>

ext2_filsys current_fs = NULL;

static int list_dir_proc(ext2_ino_t dir EXT2FS_ATTR((unused)),
			 int	entry,
			 struct ext2_dir_entry *dirent,
			 int	offset EXT2FS_ATTR((unused)),
			 int	blocksize EXT2FS_ATTR((unused)),
			 char	*buf EXT2FS_ATTR((unused)),
			 void	*private)
{
	char			name[EXT2_NAME_LEN + 1];
	int			thislen;
        struct list_dir_struct  *ls;
        
        ls = (struct list_dir_struct *)private;
	thislen = dirent->name_len & 0xFF;
	strncpy(name, dirent->name, thislen);
	name[thislen] = '\0';
	
        if((!ls) || (ls->original_name == NULL)){
            fprintf(stderr, "Error: pointer error in list_dir_proc\n");
            return 0;
        }

	if (entry == DIRENT_DELETED_FILE) {
            if(strcmp(name, ls->original_name) == 0){
                ls->ino = dirent->inode;
            }
        }

        return 0;
}

ext2_ino_t get_ino_deleted_file(char *original_name){
        ext2_ino_t inode, cwd = EXT2_ROOT_INO, root = EXT2_ROOT_INO;
        struct list_dir_struct ls;
        char * dir_name = strdup(original_name);
        int flags;
        
        original_name = basename(original_name);
        
        /** get directory inode */
        int retval = ext2fs_namei(current_fs, root, cwd, dirname(dir_name), &inode);
        if (retval) {
                return 0;
        }
        
        flags = DIRENT_FLAG_INCLUDE_EMPTY | DIRENT_FLAG_INCLUDE_REMOVED;
        ls.ino = 0;
        ls.original_name = original_name;
        
        retval = ext2fs_dir_iterate2(current_fs, inode, flags, 
                0, list_dir_proc, &ls);
        
        return ls.ino;
}

static int count_trailing_zeros(char *data, int bs){
    for (int i = bs-1; i >= 0; i--){
        if(data[i])
            return bs-(i+1);
    }
    
    return 0;
}

static int read_last_block(int fd, void *buffer, int bs){
    off_t pos;
    if ((pos = lseek(fd, 0L, SEEK_END)) < 0)
        return -1;
    
    if (lseek(fd, pos-bs, SEEK_SET) < 0)
        return -1;
    
    if (bs != read(fd, buffer, bs)){
        return -1;
    }
    
    return 0;
}

static int strip_trailing_zeros(char *output_name){
    char * block = NULL;
    int fd;
    unsigned int bs = current_fs->blocksize, zeros;
    off_t pos;
    
    block = (char *)malloc(bs);
    if(block == NULL)
        return -1;
    
    fd = open(output_name, O_RDWR, 0600);
    if(fd < 0){
        free(block);
        return -1;
    }
    
    if (read_last_block(fd, block, bs) < 0){
        free(block);
        close(fd);
        return -1;
    }
    
    zeros = count_trailing_zeros(block, bs);
    if (zeros){
        /* get file len */
        if ((pos = lseek(fd, 0L, SEEK_END)) < 0){
            free(block);
            close(fd);
            return -1;
        }
        
        /* truncate */      
        if(ftruncate(fd, pos-zeros) < 0){
            free(block);
            close(fd);
            return -1;
        }
    }
    
    close(fd);
    
    return 0;
}

static int check_extent_hdr(struct ext3_extent_header *header){
    D(printf("Checking extent header..."));
    if (header->eh_depth > EXT4_EXT_MAX_DEPTH) {
        fprintf(stderr, "Error: invalid value of eh_depth in extent header!!!\n");
        return -1;
    }
    
    if (header->eh_entries > header->eh_max) {
        fprintf(stderr, "Error: invalid value of eh_depth in extent header!!!\n");
        return -1;
    }
    
    /* TODO: add other extent checks */
    
    D(printf("OK\n"));
    return 0;
}

static int read_inode_full(ext2_ino_t ino, struct ext2_inode * inode, 
        int bufsize){
    
       int retval;

       retval = ext2fs_read_inode_full(current_fs, ino, inode, bufsize);
       if (retval) {
              fprintf(stderr, "Error: while reading inode %u\n", ino);
              return 1;
       }
       
       return 0;
}

static int close_filesystem() {
    int retval;
    
    retval = ext2fs_close(current_fs);
    if (retval)
        fprintf(stderr, "ERRROR: failed to close ext2fs\n");
    current_fs = NULL;
    
    return 0;
}

static int open_filesystem(char * device, int open_flags, blk_t superblock,
        blk_t blocksize) {

    int retval;
    io_channel data_io = 0;

    if (superblock != 0 && blocksize == 0) {
        fprintf(stderr, "if you specify the superblock, you must also specify the block size\n");
        current_fs = NULL;
        return -1;
    }

    retval = ext2fs_open(device, open_flags, superblock, blocksize,
            unix_io_manager, &current_fs);
    if (retval) {
        fprintf(stderr, "Error: can NOT open filesystem\n");
        current_fs = NULL;
        return -1;
    }

    retval = ext2fs_read_inode_bitmap(current_fs);
    if (retval) {
        fprintf(stderr, "Error: while reading inode bitmap\n");
        goto errout;
    }
    retval = ext2fs_read_block_bitmap(current_fs);
    if (retval) {
        fprintf(stderr, "Error: while reading block bitmap\n");
        goto errout;
    }

    if (data_io) {
        retval = ext2fs_set_data_io(current_fs, data_io);
        if (retval) {
            fprintf(stderr,
                    "Error: while setting data source\n");
            goto errout;
        }
    }

    return 0;

errout:
    retval = ext2fs_close(current_fs);
    if (retval)
        fprintf(stderr, "Error: while trying to close filesystem\n");
    current_fs = NULL;

    return -1;
}

static int ext4_ext_more_to_iterate(struct ext_path *path) {
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

static struct block_buffer *read_block(ext4_fsblk_t pblk){
    struct block_buffer *bb = NULL;

    bb = malloc(sizeof(struct block_buffer));
    if(bb == NULL){
        fprintf(stderr, "Error: unable to allocate enough memory!\n");
        return NULL;
    }

    bb->data = malloc(current_fs->blocksize);
    if(bb->data == NULL){
        free(bb);
        fprintf(stderr, "Error: unable to allocate enough memory!\n");
        return NULL;
    }

    if (io_channel_read_blk64(current_fs->io, pblk, 1, bb->data)) {
        block_release(bb);
        fprintf(stderr, "Error: unable to read block from device!\n");
        return NULL;
    }

    return bb;
}

void print_ext2_exhdr_info(struct ext3_extent_header *header) {
    printf("******* Printing extent header info ********\n");
    printf("eh_magic: %04X\n", header->eh_magic);
    printf("eh_entries: %d\n", header->eh_entries);
    printf("eh_max: %d\n", header->eh_max);
    printf("eh_depth: %d\n", header->eh_depth);
    printf("eh_generation: %08X\n", (header->eh_generation));
}

void print_ext2_inode(struct ext2_inode * inode) {
    printf("******* Printing inode info ********\n");
    printf("mode: %d\n", inode->i_mode);
    printf("uid: %d\n", inode->i_uid);
    printf("size: %d\n", inode->i_size);
    printf("atime: %d\n", inode->i_atime);
    printf("ctime: %d\n", inode->i_ctime);
    printf("mtime: %d\n", inode->i_mtime);
    printf("dtime: %d\n", inode->i_dtime);
    printf("gid: %d\n", inode->i_gid);
    printf("links count: %d\n", inode->i_links_count);
    printf("blocks: %d\n", inode->i_blocks);
    printf("flags: %d\n", inode->i_flags);
    printf("size_high: %d\n", inode->i_size_high);
}

static int append_undeleted_data(int out_fd, ext4_lblk_t ex_ee_block, 
                                 ext4_fsblk_t pblk, unsigned short ex_ee_len){
    void * buffer;
    int bytes_read = 0, bytes_written = 0;
    off_t write_off = 0;
    off_t out_offset = (off_t)ex_ee_block * current_fs->blocksize; 
    off_t bytes_to_transfer = ex_ee_len * current_fs->blocksize;
    
    D(printf("Entering append_undeleted_data function \n"));
    
    if(lseek(out_fd, out_offset, SEEK_SET) < 0){
        fprintf(stderr, "Error: unable to seek to position: %ld\n", out_offset);
        return -1;
    }
    
    buffer = (void *)malloc(current_fs->blocksize);
    if(buffer == NULL){
        fprintf(stderr, "Error: unable to allocate memory for buffer!\n");
        return -1;
    }
    
    while(bytes_to_transfer){
        if (io_channel_read_blk64(current_fs->io, pblk, 1, buffer)) {
            fprintf(stderr, "Error: unable to read block from device!\n");
            free(buffer);
            return -1;
        }
        bytes_read = current_fs->blocksize;
        
        while(bytes_read){
                if((bytes_written = write(out_fd, ((char *)buffer)+write_off, bytes_read-write_off)) < 0){
                    fprintf(stderr, "Error: unable to write data!\n");
                    free(buffer);
                    return -1;
                }
                bytes_read -= bytes_written;
                write_off += bytes_written;
        }
        
        bytes_to_transfer -= write_off;
        bytes_read = bytes_written = 0;
        write_off = 0;
	pblk++;
    }
    
    free(buffer);
    
    return 0;
}

static int ext4_undelete_leaf(struct ext_path *path, int out_fd) {
    struct ext3_extent *ex;
    ext4_lblk_t ex_ee_block;
    ext4_fsblk_t pblk;
    unsigned short ex_ee_len;
    struct ext3_extent_header *eh;

    if (!path->p_hdr){
        path->p_hdr = ext_block_hdr(path->p_bb);
        if (check_extent_hdr(path->p_hdr)){
                return -1;
        }

        D(print_ext2_exhdr_info(path->p_hdr));

        /* if there is value in eh_entries, try to use it! */
        if(!path->p_hdr->eh_entries){
                D(printf("Restoring eh_entries from eh_generation: %d\n", path->p_hdr->eh_entries));
                path->p_hdr->eh_entries = ext4_ext_get_entries(path->p_hdr);
        }
    }
    eh = path->p_hdr;
    
    ex = EXT_FIRST_EXTENT(eh);
    ex_ee_block = ex->ee_block;
    ex_ee_len = ext4_ext_get_actual_len(ex);
    pblk = ext4_ext_pblock(ex);
    
    /* process all leafs */
    while (ex <= EXT_LAST_EXTENT(eh)) {
        D(printf("DATA: Extent starts at block: %lld (len: %d, first block's number: %d). Processing leaf node..\n",  pblk, ex_ee_len, ex_ee_block));
        
        append_undeleted_data(out_fd, ex_ee_block, pblk, ex_ee_len);
        
        ex++;
        ex_ee_block = ex->ee_block;
        ex_ee_len = ext4_ext_get_actual_len(ex);
        pblk = ext4_ext_pblock(ex);
    }
    
    return 0;
}


static int open_out_file(char *output_name){
    int fd = open(output_name, O_CREAT | O_RDWR | O_TRUNC, 0600);
    
    return fd;
}

static void close_out_file(int out_fd){
    if(out_fd > 0)
       close(out_fd);
}

int ext4_undelete_file(struct ext2_inode * inode_buf, char *output_name) {
    struct ext_path *path = NULL;
    int depth = 0;
    int out_fd, err, i = 0;

    D(print_ext2_inode(inode_buf));
    D(print_ext2_exhdr_info(ext_inode_hdr(inode_buf)));

    if (check_extent_hdr(ext_inode_hdr(inode_buf))){
        return -1;
    }

    /* get depth from eh_generation and check it !!! */
    D(printf("Restoring eh_depth from eh_generation: %d\n", depth));
    depth = ext4_ext_get_depth(ext_inode_hdr(inode_buf)); 

    path = calloc(depth + 1, sizeof (struct ext_path));
    if (path == NULL) {
        fprintf(stderr, "Error: can't allocate enough memory!\n");
        return -1;
    }

    path[0].p_depth = depth;
    path[0].p_hdr = ext_inode_hdr(inode_buf);
    path[0].p_hdr->eh_entries = ext4_ext_get_entries(path[0].p_hdr);
    D(printf("Restoring eh_entries from eh_generation: %d\n", path[0].p_hdr->eh_entries));

    out_fd = open_out_file(output_name);
    if (out_fd < 0){
        fprintf(stderr, "Error: can't write to output file: %s!\n", output_name);
        if (path != NULL)
                free(path);

        return -1;
    }

    i = 0;
    while (i >= 0) {
        /* leaf block */
        if (i == depth) {
            D(printf("Processing leaf node..\n"));

            err = ext4_undelete_leaf(path + i, out_fd);
            if (err < 0){
                return -1;
            }

            block_release(path[i].p_bb);
            path[i].p_bb = NULL;

            i--;

            continue;
        }

        /* index block */
        if (!path[i].p_hdr) {
            path[i].p_hdr = ext_block_hdr(path[i].p_bb);
            if (check_extent_hdr(ext_inode_hdr(inode_buf))){
                return -1;
            }
        }

        if (!path[i].p_idx) {
            /* this level hasn't been touched yet */
            path[i].p_idx = EXT_FIRST_INDEX(path[i].p_hdr);

            /* if eh_entries is 0, get eh_entries form eh_generation */
            if (!path[i].p_hdr->eh_entries){
                path[i].p_hdr->eh_entries = ext4_ext_get_entries(path[i].p_hdr);
            }
        } else {
            /* we were already here, see at next index */
            path[i].p_idx++;
        }

        if (ext4_ext_more_to_iterate(path + i)) {
            struct block_buffer *bb = NULL;
            D(printf("Reading block: %lld, index: %d\n", ext_idx_pblock(path[i].p_idx), i));

            /* go to the next level */
            memset(path + i + 1, 0, sizeof (*path));

            bb = read_block(ext_idx_pblock(path[i].p_idx));
            if(bb == NULL){
                fprintf(stderr, "ERROR\n");
                return -1;
            }

            if (i + 1 > depth) {
                fprintf(stderr, "ERROR\n");
                return -1;
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

    close_out_file(out_fd);

    return 0;
}

int undelete_file(char *device, char *original_name, ext2_ino_t ino, char *output_name, bool strip) {
    int retval;
    struct ext2_inode * inode_buf;
    int open_flags = EXT2_FLAG_SOFTSUPP_FEATURES;

    /* open filesystem first */
    D(printf("Opening filesystem...\n"));
    retval = open_filesystem(device, open_flags, 0, 0);
    if (retval) {
          return -1;
    }

    // if original filename is specified
    if (original_name != NULL){
        ino = get_ino_deleted_file(original_name);
        if (ino == 0){
            fprintf(stderr, "Error: can't find inode number of specified "
                    "original filename: \"%s\"\n", original_name);
            return -1;
        }
    }

    inode_buf = (struct ext2_inode *)
                     malloc(EXT2_INODE_SIZE(current_fs->super));

    if (!inode_buf) {
        fprintf(stderr, "Error: can't allocate buffer for ext2_inode\n");
        close_filesystem();
        return -1;
    }

    /* try to read inode data */
    D(printf("Trying to read inode: %d...\n", ino));
    if (read_inode_full(ino, inode_buf, 
            EXT2_INODE_SIZE(current_fs->super))) {
          free(inode_buf);
          close_filesystem();
          return -1;
    }

    /* check, if the file still exists */
    if (inode_buf->i_links_count){
        fprintf(stderr, "Error: Can't undelete file (file exists)!\n");
        free(inode_buf);
        close_filesystem();
        return -1;
    }

    /* check, if the file is regular file */
    if (!LINUX_S_ISREG(inode_buf->i_mode)){
        fprintf(stderr, "Error: Can't undelete file (deleted file is "
                "not regular file)!\n");
        free(inode_buf);
        close_filesystem();
        return -1;
    }

    /* undelete file */
    if (ext4_undelete_file(inode_buf, output_name) != 0) {
        fprintf(stderr, "Error: unable to undelete file!\n");
        free(inode_buf);
        close_filesystem();
        return -1;
    }

    /* free buffer */
    free(inode_buf);

    if(strip)
        strip_trailing_zeros(output_name);

    /* close filesystem */
    D(printf("Closing filesystem...\n"));
    if(close_filesystem() < 0){
        return -1;
    }

    return 0;
}
