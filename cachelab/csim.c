#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

/* Parse address */
#define GET_TAG(addr)		(addr >> (s+b))
#define GET_SET(addr)		((addr>>b) & ~(ULLONG_MAX<<s))

/* Constant */
#define SET_COUNT	(1<<s)

/* Cache line struct */
struct line {
	bool valid;
	unsigned long long tag;
	unsigned long long lru_value;
};

/* Global varient  */
static int hit_count = 0, miss_count = 0, eviction_count = 0;	/* Summary */
static int s, b, E;												/* Option */
static int verbose = 0;											/* Verbose */
static char const *tracefile;									/* Trace file */
static struct line **cache;										/* Cache entry */

/* Extern function */
int getopt(int argc, char const *argv[], const char *optstring);

/*
 * printUsage - print usage
 */
static void print_usage(char const *exename)
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
static void cache_access(unsigned long long addr)
{
	unsigned long long tag = GET_TAG(addr);	/* Get tag */
	int set = GET_SET(addr);				/* Get set index */
	struct line* lines = cache[set];		/* Get set entry */
	unsigned long long min = ULLONG_MAX;	
	int aim = -1, first_empty = -1, lru = -1;
	for (int i = 0; i < E; ++i) {
		/* Find hit line */
		if (lines[i].valid && lines[i].tag == tag)
			aim = i;
		/* Decrease LRU value */
		if (lines[i].valid)
			lines[i].lru_value--;
		/* Find unvaliable line */
		if (!lines[i].valid && first_empty == -1)
			first_empty = i;
		/* Find LRU line */
		if (lines[i].valid && lines[i].lru_value <= min) {
			lru = i;
			min = lines[i].lru_value;
		}
	}
	/* Hit */
	if (aim != -1) {
		lines[aim].lru_value = ULLONG_MAX;
		hit_count++;
		if (verbose)
			printf(" hit");
		return;
	}
	/* Miss */
	miss_count++;
	if (first_empty != -1) {
		lines[first_empty].valid = 1;
		lines[first_empty].tag = tag;
		lines[first_empty].lru_value = ULLONG_MAX;
		if (verbose)
			printf(" miss");
		return;
	}
	/* Eviction */
	lines[lru].valid = 1;
	lines[lru].tag = tag;
	lines[lru].lru_value = ULLONG_MAX;
	eviction_count++;
	if (verbose)
		printf(" miss eviction");
}

int main(int argc, char const *argv[])
{
	/* Parse arguments */
	int result;
	extern char *optarg;
	while ((result = getopt(argc, argv, "hvs:E:b:t:")) != EOF) {
		switch (result) {
			case 'h':	/* Optional help flag that print usage info */
				print_usage(argv[0]);
				return 0;
			case 'v':	/* Optional verbose flag that display trace info */
				verbose = 1;
				break;
			case 's':	/* Number of set index bits */
				s = atoi(optarg);
				break;
			case 'E':	/* Associativity (number of line per set) */
				E = atoi(optarg);
				break;
			case 'b':	/* Number of block bits */
				b = atoi(optarg);
				break;
			case 't':	/* Name of trace file */
				tracefile = optarg;
				break;
			case '?':	/* Get argumnets failed */
				print_usage(argv[0]);
				return 1;
		}
	}

	/* Check options */
	if (!s || !E || !b) {
		printf("%s: Missing required command line argument\n", argv[0]);
		print_usage(argv[0]);
		return 1;
	} 

	/* Open trace file */
	FILE *fp;
	if ((fp = fopen(tracefile, "r")) == NULL) {
		printf("%s: No such file or directory\n", tracefile);
		return 1;
	}

	/* Build cache */
	cache = (struct line**) malloc(SET_COUNT*sizeof(struct line*)); 
	for (int i = 0; i < SET_COUNT; ++i) {
		cache[i] = (struct line*) malloc(E*sizeof(struct line));
		/* Initial cache */
		for (int j = 0; j < E; ++j) {
			cache[i][j].valid = 0;
			cache[i][j].tag = ULLONG_MAX;
		}
	}

	/* Load instrument */
	char instr;
	unsigned long long addr;
	int size;
	while (fscanf(fp, " %c %llx,%d", &instr, &addr, &size) != EOF) {
		if (verbose)
			printf("%c %llx,%d", instr, addr, size);
		switch (instr) {
			case 'M':	/* Data modify */
				cache_access(addr);
			case 'L':	/* Data load */
			case 'S':	/* Data store */
				cache_access(addr);
				break;
		}
		if (verbose)
			printf("\n");
	}

	/* Free cache */
	for (int i = 0; i < SET_COUNT; ++i)
		free(cache[i]);
	free(cache);

	/* Close file */
	fclose(fp);

	/* Print summary */
	printSummary(hit_count, miss_count, eviction_count);
	return 0;
}
