#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <asm/byteorder.h>
#include "main.h"
#include "ext4.h"
#include "ext4_extents.h"

#define GETOPT_ARGS "hi:o:"

static void print_exhdr_info(struct ext4_extent_header *header){
	printf("******* Printing extent header info ********\n");
	printf("eh_magic: %X\n", header->eh_magic);
	printf("eh_entries: %d\n", header->eh_entries);
	printf("eh_max: %d\n", header->eh_max);
	printf("eh_depth: %d\n", header->eh_depth);
	printf("eh_generation: %X %X\n", header->eh_generation, ((char)(header->eh_generation))+2);
}

static void print_inode_info(struct ext4_inode * inode){
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

static void print_help(){
	printf("\nUsage: ./ext4_undel device -i inode_offset [OPTIONS]");
	printf("\n\nOptions:");
	printf("\n\t-h: print this help");
	printf("\n\t-i inode_offset: specifies offset of inode, which holds info about file which should be undeleted");
	printf("\n\t-o filename: filename of undeleted file. Default is %s", DEFAULT_FILENAME);
	printf("\n");
}

static struct options parse_options(int argc, char ** argv){
	int c;
	int index;

	struct options opts = {
		.state = 0,
		.inode_offset = -1,
		.output_name = DEFAULT_FILENAME,
		.device = NULL
	};

	while ((c = getopt (argc, argv, GETOPT_ARGS)) != -1) {
		switch (c) {
			case 'h':
				print_help();
				break;
			case 'i':
				// TODO: check value here
				opts.inode_offset = atoi(optarg);
				break;
			case 'o':
				opts.output_name = optarg;
				break;
			case '?':
				if (optopt == 'c')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return opts;
			default:
				print_help();
				opts.state = -1;
				return opts;
		}
	}
	
	argc -= optind;
	argv += optind;

	// process non option params
	for (index = 0; index < argc; index++){
		if (index == 0) {
			opts.device = argv[index];
		}
	}

	// device param is required
	if (opts.device == NULL){
		opts.state = -1;
		fprintf(stderr, "Missing device param!\n");
		print_help();
		return opts;
	}

	// inode offset is required
	if (opts.inode_offset == -1){
		opts.state = -1;
		fprintf(stderr, "inode_offset param is required!\n");
		print_help();
		return opts;
	}

	return opts;
}

int main(int argc, char ** argv){
    int ret;
    struct options opts;
    struct ext4_inode inode;
    unsigned long int block_size;
    void * block;

    opts = parse_options(argc, argv);
    if (opts.state < 0)
	return EXIT_FAILURE;

    int fd = open(opts.device, O_RDONLY);
    if (fd < 0){
	printf("Err: file not found!\n");
	return 0;
    }

    if(lseek64(fd, opts.inode_offset, SEEK_SET) < 0)
	printf("ERROR!!!!\n"); //TODO: better error handling
    
    if (read(fd, &inode, sizeof(struct ext4_inode)) < 0)
	printf("ERROR!!!!\n"); //TODO: better error handling

    print_inode_info(&inode);
    print_exhdr_info(ext_inode_hdr(&inode));
    //if(posix_memalign(&block, EXT4_BLOCK_SIZE, EXT4_BLOCK_SIZE))
    //		printf("ERROR!!!\n");

    // first, read inode by given offset
    // then start iterating the tree
    // try to reconstruct file...
    int i = 0;
    int depth = ext_depth(&inode); // get depth from eh_generation and check it !!!
    struct ext4_ext_path *path = NULL;
    
    path = calloc(depth+1,sizeof(struct ext4_ext_path));
    if (path == NULL)
	printf("ERROR!!!!\n"); // TODO: better error handling

    path[0].p_depth = depth;
    path[0].p_hdr = ext_inode_hdr(&inode);
    i = 0;

    while (i >= 0) {
	// leaf block
	if (i == depth) {
		
		continue;
	}

	// index block
	if (!path[i].p_hdr) {
		path[i].p_hdr = ext_block_header(/* block address */);
	}

    }

    if (path != NULL)
        free(path);
    close(fd);

    
    // ext4_lookup
    // ext4_igetí

    return EXIT_SUCCESS;
}