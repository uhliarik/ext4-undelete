#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "main.h"

#define GETOPT_ARGS "abc:"

static void print_help(){
	printf("\nUsage: ./ext4_undel [OPTIONS]");
	printf("\n\nOptions:");
	printf("\n\t-h: print this help");
}

static int parse_options(int argc, char ** argv){
	int c;
	int index;

	while ((c = getopt (argc, argv, GETOPT_ARGS)) != -1)
		switch (c) {
			case 'a':
				//aflag = 1;
				break;
			case 'b':
				//bflag = 1;
				break;
			case 'c':
				//cvalue = optarg;
				break;
			case '?':
				if (optopt == 'c')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
			default:
				print_help();
				return -1;
		}

	//printf ("aflag = %d, bflag = %d, cvalue = %s\n", aflag, bflag, cvalue);

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);

}

int main(int argc, char ** argv){
    int ret;

    ret = parse_options(argc, argv);

    return EXIT_SUCCESS;
}