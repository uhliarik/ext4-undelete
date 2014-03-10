#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "fragmented.h"

#define GETOPT_ARGS "hb:c:d:"

static void print_help() {
    printf("\nUsage: ./fragmented file [OPTIONS]");
    printf("\n\nOptions:");
    printf("\n\t-h: print this help");
    printf("\n\t-b block_size: specifies block size, default value is %d B", DEFAULT_BLOCKSIZE);
    printf("\n\t-c extents_count: how many extents will be created. Default value is %d", DEFAULT_EXTENT_COUNT);
    printf("\n\t-d data: specify which byte will fill all blocks. Possible values are from range 0-255. Default value is %02X", (char)DEFAULT_DATA_VAL);
    printf("\n");
}

static struct options parse_options(int argc, char ** argv) {
    int c;
    int index;

    struct options opts = {
        .state = 0,
        .bs = DEFAULT_BLOCKSIZE,
        .extent_count = DEFAULT_EXTENT_COUNT,
        .filename = NULL,
        .data_val = DEFAULT_DATA_VAL
    };
    
    while ((c = getopt(argc, argv, GETOPT_ARGS)) != -1) {
        switch (c) {
            case 'h':
                print_help();
                opts.state = -1;
                return opts;
            case 'b':          
                opts.bs = strtoul(optarg, NULL, 0);
                break;
            case 'c':
                opts.extent_count = strtoul(optarg, NULL, 0);;
                break;
            case 'd':
                opts.data_val = strtoul(optarg, NULL, 0);
                if (opts.data_val > 255){
                    opts.state = -1;
                    return opts;
                }
                break;
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                
                print_help();
                opts.state = -1;
                    
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
    for (index = 0; index < argc; index++) {
        if (index == 0) {
            opts.filename = argv[index];
        }
    }

    // filename param is required
    if (opts.filename == NULL) {
        opts.state = -1;
        fprintf(stderr, "Missing filename param!\n");
        print_help();
        return opts;
    }

    return opts;
}



int main(int argc, char **argv){
        struct options opts;

        opts = parse_options(argc, argv);
        if (opts.state < 0)
            return EXIT_FAILURE;

        void *buffer;
        if (posix_memalign(&buffer, opts.bs, opts.bs) < 0){
            return EXIT_FAILURE;
        }

        memset(buffer, opts.data_val, opts.bs); 

        int f = open(opts.filename, O_CREAT|O_TRUNC|O_WRONLY|O_DIRECT|O_LARGEFILE, 0600);
        if (f < 0){
            free(buffer);
            return EXIT_FAILURE;
        }

        for(unsigned long int i = 0; i < opts.extent_count; i++){
                if (write(f, buffer, opts.bs) < 0){
                    free(buffer);
                    return EXIT_FAILURE;
                }
                lseek64(f, opts.bs, SEEK_CUR);
        }

        close(f);
        free(buffer);

        return EXIT_SUCCESS;
}

