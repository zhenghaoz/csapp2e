#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>

/* Global varient  */

static int hit_count;
static int miss_count;
static int eviction_count;		
static int verbose;			

/* Linked List */

struct list_entry {
    struct list_entry *prev;
    struct list_entry *next;
};

#define list_next(le)	((le)->next)
#define list_prev(le)	((le)->prev)
#define list_empty(le)	((le)==(le)->prev&&(le)==(le)->next)

static void list_init(struct list_entry *le) {
    le->prev = le->next = le;
}

static void list_add(struct list_entry *li, struct list_entry *le) {
    le->next = li->next;
    le->next->prev = le;
    le->prev = li;
    le->prev->next = le;
}

#define list_add_after(li,le) (list_add(li,le))
#define list_add_before(li,le) (list_add(list_prev(li),le))

static void list_remove(struct list_entry *le) {
    le->prev->next = le->next;
    le->next->prev = le->prev;
    list_init(le);
}

/* LRU Set */

struct line {
	bool valid;
	uint64_t tag;
	struct list_entry list;
};

#define le2line(lep)	((struct line*)((void*)(lep)-offsetof(struct line,list)))

struct set
{
	size_t E;
	struct list_entry head;
	struct line *lines;
};

static void set_init(struct set *setp, size_t E) {
	setp->E = E;
	list_init(&(setp->head));
	setp->lines = (struct line *) malloc(sizeof(struct line) * E);
	for (int i = 0; i < E; i++) {
		setp->lines[i].valid = 0;
		setp->lines[i].list.prev = &(setp->lines[(i-1)%E].list);
		setp->lines[i].list.next = &(setp->lines[(i+1)%E].list);
	}
	list_add_before(&(setp->lines[0].list), &(setp->head));
}

static void set_destroy(struct set *setp) {
	free(setp->lines);
}

static void set_access(struct set *setp, uint64_t tag) {
	// Hit
	for (int i = 0; i < setp->E; i++)
		if (setp->lines[i].valid && setp->lines[i].tag == tag) {
			hit_count ++;
			if (verbose)
				printf(" hit");
			struct list_entry *lep = &(setp->lines[i].list);
			list_remove(lep);
			list_add_before(&(setp->head), lep);
			return;
		}
	// Miss
	miss_count ++;
	if (verbose)
		printf(" miss");
	struct list_entry *lep = list_next(&(setp->head));
	struct line *lp = le2line(lep);
	if (lp->valid) {
		eviction_count ++;
		if (verbose)
			printf(" eviction");
	}
	lp->valid = 1;
	lp->tag = tag;
	list_remove(lep);
	list_add_before(&(setp->head), lep);
}

/* Cache */

#define addr_tag(cachep, addr)	((addr)>>((cachep)->s+(cachep)->b))
#define addr_set(cachep, addr)	(((addr)>>(cachep)->b) & ~(ULLONG_MAX<<(cachep)->s))
#define set_count(cachep)		(1<<(cachep)->s)

struct cache {
	int s, b, E;
	struct set *sets;
};

static struct cache *cache_create(int s, int b, int E) {
	struct cache *cachep = (struct cache *) malloc(sizeof(struct cache *));
	cachep->s = s;
	cachep->b = b;
	cachep->E = E;
	cachep->sets = (struct set *) malloc(sizeof(struct set) * set_count(cachep));
	for (int i = 0; i < set_count(cachep); i++)
		set_init(&(cachep->sets[i]), E);
	return cachep;
}

static void cache_free(struct cache *cachep) {
	for (int i = 0; i < set_count(cachep); i++)
		set_destroy(&(cachep->sets[i]));
	free(cachep->sets);
}

static void cache_access(struct cache *cachep, uint64_t addr) {
	uint64_t set = addr_set(cachep, addr);
	uint64_t tag = addr_tag(cachep, addr);
	set_access(&(cachep->sets[set]), tag);
}

static void help(char const *exename)
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
	/* Extern variant and function */
	extern char *optarg;
	int getopt(int argc, char const *argv[], const char *optstring);

	/* Options */
	int s, E, b;
	char *tracefile;

	/* Parse arguments */
	int c;
	while ((c = getopt(argc, argv, "hvs:E:b:t:")) != EOF) {
		switch (c) {
			case 'h':	/* Optional help flag that print usage info */
				help(argv[0]);
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
				help(argv[0]);
				return 1;
		}
	}

	/* Check options */
	if (!s || !E || !b || !tracefile) {
		printf("%s: Missing required command line argument\n", argv[0]);
		help(argv[0]);
		return 1;
	} 

	/* Open trace file */
	FILE *fp;
	if ((fp = fopen(tracefile, "r")) == NULL) {
		printf("%s: No such file or directory\n", tracefile);
		return 1;
	}

	/* Build cache */
	struct cache *cachep = cache_create(s, b, E);

	/* Load instrument */
	char instr;
	uint64_t addr;
	int size;
	while (fscanf(fp, " %c %lx,%d", &instr, &addr, &size) != EOF) {
		if (verbose)
			printf("%c %lx,%d", instr, addr, size);
		switch (instr) {
			case 'M':	/* Data modify */
				cache_access(cachep, addr);
			case 'L':	/* Data load */
			case 'S':	/* Data store */
				cache_access(cachep, addr);
				break;
		}
		if (verbose)
			printf("\n");
	}

	/* Free cache */
	cache_free(cachep);

	/* Close file */
	fclose(fp);

	/* Print summary */
	printSummary(hit_count, miss_count, eviction_count);
	return 0;
}