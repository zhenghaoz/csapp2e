#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Parse address */
#define GET_TAG(addr)		(addr >> (s+b))
#define GET_OFFSET(addr)	(addr & ~(-1L<<b))
#define GET_SET(addr)		((addr>>b) & ~(-1L<<s))

/* Cache line struct */
struct line {
	bool valid;
	unsigned long long tag;
};

/* Varinet */
static int hit_count = 0, miss_count = 0, eviction_count = 0;	/* Summary */
static int s, b, E;											/* Option */
static struct line **cache;									/* Cache entry */

/*
 * printUsage - print usage
 */
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

/*
 * load - load data
 */
static void load(unsigned long long addr)
{

}

/*
 * save - save data
 */
static void save(unsigned long long addr)
{

}

/*
 * modify - modify data
 */
static void modify(unsigned long long addr)
{

}

int main(int argc, char const *argv[])
{
	char instr;
	unsigned long long address, size;
	char const *tracefile;	
	FILE *fp;

	/* Parse arguments */
	int i;
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h")) {	/* Help */
			printUsage(argv[0]);
			return 0;
		} else if (!strcmp(argv[i], "-v"))	/* Optional verbose flag */
			verbose = 1;
		else if (!strcmp(argv[i], "-s")) {	/* Number of set index bits */
			if (!(setnum = atoi(argv[i+1]))) {
				printf("%s: Missing required command line argument\n", argv[0]);
				printUsage(argv[0]);
				return 1;
			}
		} else if (!strcmp(argv[i], "-E")) {/* Number of line per set */
			if (!(lnnum = atoi(argv[i+1]))) {
				printf("%s: Missing required command line argument\n", argv[0]);
				printUsage(argv[0]);
				return 1;
			}
		} else if (!strcmp(argv[i], "-b")) {/* Number of block offset bits */
			if (!(blksize = atoi(argv[i+1]))) {
				printf("%s: Missing required command line argument\n", argv[0]);
				printUsage(argv[0]);
				return 1;
			}
		} else if (!strcmp(argv[i], "-t")) {/* Trace file */
			if ((tracefile = argv[++i]) == NULL) {
				printf("%s: option requires an argument -- 't'\n", argv[0]);
				printUsage(argv[0]);
				return 1;
			}
		}
	}

	/* Check options */
	if (!setnum || !lnnum || !blksize) {
		printf("%s: option requires an argument -- 't'\n", argv[0]);
		printUsage(argv[0]);
		return 1;
	}

	/* Open trace file */
	if ((fp = fopen(tracefile, "r")) == NULL) {
		printf("%s: No such file or directory\n", tracefile);
		return 1;
	}

	/* Build cache */
	cache = (struct line**) malloc(setnum*sizeof(struct line*)); 
	for (int i = 0; i < setnum; ++i) {
		cache[i] = (struct line*) malloc(lnnum*sizeof(struct line));
		/* Initial cache */
		for (int j = 0; j < lnnum; ++j)
			cache[i][j].valid = 0;
	}

	/* Load instrument */
	while (fscanf(fp, " %c %llu,%llu", &instr, &address, &size) != EOF) {
		if (verbose)
			printf("%c %llu,%llu\n", instr, address, size);
		switch (instr) {
			case 'M':	/* Data modify */
				break;
			case 'L':	/* Data load */
				break;
			case 'S':	/* Data store */
				break;
		}
	}

	/* Free cache */
	for (int i = 0; i < setnum; ++i)
		free(cache[i]);
	free(cache);

	/* Close file */
	fclose(fp);

	/* Print summary */
	printSummary(hit_count, miss_count, eviction_count);
	return 0;
}
