#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int verbose = 0, setsize, lnsize, blksize;
char const *tracefile;

static void printUsage(char const *exename)
{
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", exename);
	printf("Options:\n");
	printf("  -h         Print this help message.\n");
	printf("  -v         Optional verbose flag.\n");
	printf("  -s <num>   Number of set index bits.\n");
	printf("  -E <num>   Number of lines per set.\n");
	printf("  -b <num>   Number of block offset bits.\n");
	printf("  -t <file>  Trace file.\n\n");
	printf("Examples:\n");
	printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", exename);
	printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", exename);	
}

int main(int argc, char const *argv[])
{
	/* Parse arguments */
	int i;
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h")) {	/* Help */
			printUsage(argv[0]);
			return 0;
		} else if (!strcmp(argv[i], "-v"))	/* Optional verbose flag */
			verbose = 1;
		else if (!strcmp(argv[i], "-s")) {	/* Number of set index bits */
			if (!(setsize = atoi(argv[i+1]))) {
				printf("%s: Missing required command line argument\n", argv[0]);
				printUsage(argv[0]);
			}
		} else if (!strcmp(argv[i], "-E")) {/* Number of line per set */
			if (!(lnsize = atoi(argv[i+1]))) {
				printf("%s: Missing required command line argument\n", argv[0]);
				printUsage(argv[0]);
			}
		} else if (!strcmp(argv[i], "-b")) {/* Number of block offset bits */
			if (!(blksize = atoi(argv[i+1]))) {
				printf("%s: Missing required command line argument\n", argv[0]);
				printUsage(argv[0]);
			}
		} else if (!strcmp(argv[i], "-t")) {/* Trace file */
			if ((tracefile = argv[++i]) == NULL) {
				printf("%s: option requires an argument -- 't'\n", argv[0]);
				printUsage(argv[0]);
			}
		}
	}
	printSummary(0, 0, 0);
	return 0;
}
