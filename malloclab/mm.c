/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Team-Z",
    /* First member's full name */
    "Zhang Zhenghao",
    /* First member's email address */
    "zhangzhenghao@zjut.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* Basic constants and macros */
#define WSIZE       4
#define DSIZE       8
#define QSIZE		16
#define CHUNKSIZE   (1<<12)
#define SEG_MAX		14

#define MAX(x, y)   (x > y ? x : y)
#define MIN(x, y)	(x < y ? x : y)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)      (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))

/* Read the size and allocated fields from adress p */
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char*)(bp) - WSIZE)
#define FTRP(bp)    ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

/* Given block ptr bp, compute address of next and previous free blocks */
#define PREV_FREE_BLKP(bp)	(*(unsigned int*)bp)
#define NEXT_FREE_BLKP(bp)	(*((unsigned int*)bp + 1))

/* Segregated free list entry */
static unsigned int *free_listp = NULL;

/*
 * get_set_index - given a size, return the index of segregated free list entry
 */
static int get_set_index(size_t size)
{
	size_t temp = size - 1;
	if (temp & 0xfffff000)	return 13;
	if (temp & 0x800)		return 12;
	if (temp & 0x400)		return 11;
	if (temp & 0x200)		return 10;
	if (temp & 0x100)		return 9;
	if (temp & 0x80)		return 8;
	if (temp & 0x40)		return 7;
	if (temp & 0x20)		return 6;
	if (temp & 0x10)		return 5;
	if (temp & 0x8)			return 4;
	if (temp & 0x4)			return 3;
	if (temp & 0x2)			return 2;
	if (temp & 0x1)			return 1;
	return 0;
}

/*
 * insert_free_block - insert a free block into segregated free list
 */
static void insert_free_block(void *bp)
{
	int index = get_set_index(GET_SIZE(HDRP(bp)));
	NEXT_FREE_BLKP(bp) = free_listp[index];
	if (NEXT_FREE_BLKP(bp))
		PREV_FREE_BLKP(NEXT_FREE_BLKP(bp)) = (unsigned int)bp;
	free_listp[index] = (unsigned int)bp;
}

/*
 * remove_free_block - remove a free block in the segregated free lsit
 */
static void remove_free_block(void *bp)
{
	int index = get_set_index(GET_SIZE(HDRP(bp)));
	if (free_listp[index] == (unsigned int)bp) {	/* Free block is the first node */
		free_listp[index] = NEXT_FREE_BLKP(bp);
	} else {										/* Free block is not the first node */
		NEXT_FREE_BLKP(PREV_FREE_BLKP(bp)) = NEXT_FREE_BLKP(bp);
		if (NEXT_FREE_BLKP(bp))
			PREV_FREE_BLKP(NEXT_FREE_BLKP(bp)) = PREV_FREE_BLKP(bp);
	}
}

/*
 * coalesce - coalesce previous or next free blocks
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {         /* Case 1 */
    	insert_free_block(bp);
        return bp;
    } else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_free_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_free_block(bp);
    } else if (!prev_alloc && next_alloc) { /* Case 3*/
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        remove_free_block(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_free_block(bp);
    } else {                                /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_free_block(bp);
    }
    return bp;
}

/*
 * extend_heap - extends the heap with a new free block
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));           /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));           /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   /* New epilogue header*/

    /* Coalesce if the prvious block was free */
    return coalesce(bp);
}

/*
 * find_fit - find suitable free block
 */
static void *find_fit(size_t asize)
{
	void *search_listp = NULL;
	int index = get_set_index(asize);	/* Start search entry */
	while (index < SEG_MAX) {
		search_listp = (void*) free_listp[index];
		while (search_listp) {
			if (GET_SIZE(HDRP(search_listp)) >= asize)
				return search_listp;
			search_listp = (void*) NEXT_FREE_BLKP(search_listp);
		}
		index++;						/* Can't find suitable free block, search in bigger segregated list */
	}

    /* Available block not found */
    return NULL;
}

/*
 * place - update information of a free block
 */
static void place(void *bp, size_t asize)
{
    int free_size = GET_SIZE(HDRP(bp));
    int use_size = (free_size - asize >= QSIZE) ? asize : free_size;
    int left_size = free_size - use_size;

    remove_free_block(bp);

    /* Place block */
    PUT(HDRP(bp), PACK(use_size, 1));
    PUT(FTRP(bp), PACK(use_size, 1));

    /* Place left block */
    if (left_size) {
        PUT(HDRP(NEXT_BLKP(bp)), PACK(left_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(left_size, 0));
        insert_free_block(NEXT_BLKP(bp));
    }
}

static int mm_check()
{
	/* Check whether all free blocks have been coalesced */
	void *heap_listp = mem_heap_lo() + DSIZE;
	int last_alloc = 1;
	while (GET_SIZE(HDRP(heap_listp))) {
		if (!last_alloc && !GET_ALLOC(HDRP(heap_listp))) {
			printf("There is some free blocks weren't coalesced.\n");
			return -1;
		}
		last_alloc = GET_ALLOC(HDRP(heap_listp));
		heap_listp = NEXT_BLKP(heap_listp);
	}
	/* Check whether all free blocks in right segregated list */
	int i, high, low;
	heap_listp = (void*)free_listp[0];
	while (heap_listp) {
		if (GET_SIZE(HDRP(heap_listp)) != 1) {
			printf("There is some free blocks weren't in the right segregated list.\n");
			return -1;
		}
		heap_listp = (void*)NEXT_FREE_BLKP(heap_listp);
	}
	for (i = 1; i < SEG_MAX-1; ++i) {
		high = 1<<i;
		low = (1<<(i-1))+1;
		heap_listp = (void*)free_listp[i];
		while (heap_listp) {
			if (GET_SIZE(HDRP(heap_listp)) < low || GET_SIZE(HDRP(heap_listp)) > high) {
				printf("There is some free blocks weren't in the right segregated list.\n");
				return -1;
			}
			heap_listp = (void*)NEXT_FREE_BLKP(heap_listp);
		}
	}
	heap_listp = (void*)free_listp[SEG_MAX-1];
	while (heap_listp) {
		low = (1<<12)+1;
		if (GET_SIZE(HDRP(heap_listp)) < low) {
			printf("There is some free blocks weren't in the right segregated list.\n");
			return -1;
		}
		heap_listp = (void*)NEXT_FREE_BLKP(heap_listp);
	}
	return 0;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *heap_listp = NULL;
    int i;

    /* Create the initial heap */
    if ((heap_listp = mem_sbrk(18*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);                         	/* Alignment padding */
    PUT(heap_listp + (WSIZE), PACK(16*WSIZE, 1));  	/* Prologue header */
    PUT(heap_listp + (16*WSIZE), PACK(16*WSIZE, 1));/* Prologue footer */
    PUT(heap_listp + (17*WSIZE), PACK(0, 1));    	/* Epilogue header */

    /* Init segregated free list */
    for (i = 2; i < 16; ++i) 
    	PUT(heap_listp + (i*WSIZE), 0);
    free_listp = heap_listp + (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKS bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;	
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;        /* Adjusted block size */
    size_t extendsize;   /* Amount to extend a heap if no fit */
    char *bp;

    /* Ignore spurious request */
    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search for free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *newptr;
    size_t old_size, asize, copy_size, prev_alloc, prev_size, next_alloc, next_size, use_size, left_size;

    /* If ptr equals to NULL, do malloc */
    if (ptr == NULL)
        return mm_malloc(size);

    /* If size equals to 0, do free */
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    /* Calculate new block size */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Get information about block size and allocate status */
    next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    prev_size = GET_SIZE(HDRP(PREV_BLKP(ptr)));
    old_size = GET_SIZE(HDRP(ptr));
    copy_size = MIN(old_size - DSIZE, size);

    /* New block size is equal or smaller than old block */
    if (asize <= old_size) {           
    	size_t use_size = (old_size - asize >= QSIZE) ? asize : old_size;
    	size_t left_size = old_size - use_size;
    	PUT(HDRP(ptr), PACK(use_size, 1));
    	PUT(FTRP(ptr), PACK(use_size, 1));
    	if (left_size) {
    		PUT(HDRP(NEXT_BLKP(ptr)), PACK(left_size, 0));
    		PUT(FTRP(NEXT_BLKP(ptr)), PACK(left_size, 0));
    		insert_free_block(NEXT_BLKP(ptr));
    	}
        return ptr;
    } 

    /* New block size is bigger than old block */
    if (!next_alloc && prev_alloc && old_size + next_size >= asize) {	/* Merge with next free block */
    	remove_free_block(NEXT_BLKP(ptr));
    	use_size = (old_size + next_size - asize >= QSIZE) ? asize : (old_size + next_size);
    	left_size = old_size + next_size - use_size;
    	PUT(HDRP(ptr), PACK(use_size, 1));
    	PUT(FTRP(ptr), PACK(use_size, 1));
    	if (left_size) {
    		PUT(HDRP(NEXT_BLKP(ptr)), PACK(left_size, 0));
    		PUT(FTRP(NEXT_BLKP(ptr)), PACK(left_size, 0));
    		insert_free_block(NEXT_BLKP(ptr));
    	}
    	return ptr;
    } else if (!prev_alloc && next_alloc && old_size + prev_size >= asize) {	/* Merge with previous free block */
    	newptr = PREV_BLKP(ptr);
    	remove_free_block(newptr);
    	use_size = (old_size + prev_size - asize >= QSIZE) ? asize : (prev_size + old_size);
    	left_size = old_size + prev_size - use_size;
    	PUT(HDRP(newptr), PACK(use_size, 1));
    	memcpy(newptr, ptr, copy_size);
    	PUT(FTRP(newptr), PACK(use_size, 1));
    	if (left_size) {
    		PUT(HDRP(NEXT_BLKP(newptr)), PACK(left_size, 0));
    		PUT(FTRP(NEXT_BLKP(newptr)), PACK(left_size, 0));
    		insert_free_block(NEXT_BLKP(newptr));
    	}
    	return newptr;
    } else if (!prev_alloc && !next_alloc && prev_alloc + old_size + next_alloc >= asize) {	/* Merge with previous and next free block */
    	remove_free_block(PREV_BLKP(ptr));
    	remove_free_block(NEXT_BLKP(ptr));
    	use_size = (next_alloc + old_size + prev_size - asize >= QSIZE) ? asize : (next_alloc + prev_size + old_size);
    	left_size = next_alloc + old_size + prev_size - use_size;
    	newptr = PREV_BLKP(ptr);
    	PUT(HDRP(newptr), PACK(use_size, 1));
    	memcpy(newptr, ptr, copy_size);
    	PUT(FTRP(newptr), PACK(use_size, 1));
    	if (left_size) {
    		PUT(HDRP(NEXT_BLKP(newptr)), PACK(left_size, 0));
    		PUT(FTRP(NEXT_BLKP(newptr)), PACK(left_size, 0));
    		insert_free_block(NEXT_BLKP(newptr));
    	}
    	return newptr;
    }

    /* Malloc new space */
    if ((newptr = mm_malloc(size)) == NULL)
    	return NULL;
    memcpy(newptr, ptr, copy_size);
    mm_free(ptr);

    return newptr;
}