#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "main.h"
#include "ext4_undelete.h"

#define GETOPT_ARGS "hi:o:"

static void print_help() {
    printf("\nUsage: ./ext4_undel device -i inode_offset [OPTIONS]");
    printf("\n\nOptions:");
    printf("\n\t-h: print this help");
    printf("\n\t-i inode_offset: specifies offset of inode, which holds info about file which should be undeleted");
    printf("\n\t-o filename: filename of undeleted file. Default is %s", DEFAULT_FILENAME);
    printf("\n");
}

static struct options parse_options(int argc, char ** argv) {
    int c;
    int index;

    struct options opts = {
        .state = 0,
        .inode_offset = -1,
        .output_name = DEFAULT_FILENAME,
        .device = NULL
    };

    while ((c = getopt(argc, argv, GETOPT_ARGS)) != -1) {
        switch (c) {
            case 'h':
                print_help();
                opts.state = -1;
                return opts;
            case 'i':
                // TODO: check value here
                opts.inode_offset = atoi(optarg);
                break;
            case 'o':
                opts.output_name = optarg;
                break;
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
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
            opts.device = argv[index];
        }
    }

    // device param is required
    if (opts.device == NULL) {
        opts.state = -1;
        fprintf(stderr, "Missing device param!\n");
        print_help();
        return opts;
    }

    // inode offset is required
    if (opts.inode_offset == -1) {
        opts.state = -1;
        fprintf(stderr, "inode_offset param is required!\n");
        print_help();
        return opts;
    }

    return opts;
}

int main(int argc, char ** argv) {
    struct options opts = parse_options(argc, argv);
    if (opts.state < 0)
        return EXIT_FAILURE;

    ext4_undelete_file(opts.device, opts.inode_offset, opts.output_name);

    return EXIT_SUCCESS;
}