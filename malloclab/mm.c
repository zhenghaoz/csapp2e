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
#define CHUNKSIZE   (1<<12)

#define MAX(x, y)   (x > y ? x : y)

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

static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {         /* Case 1 */
        return bp;
    } else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { /* Case 3*/
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
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

static void *find_fit(size_t asize)
{
    int block_size;
    void *heap_listp = mem_heap_lo() + DSIZE;

    /* Find available block */
    while ((block_size = GET_SIZE(HDRP(heap_listp)))) {
        if (block_size >= asize && GET_ALLOC(HDRP(heap_listp)) == 0)
            return heap_listp;
        heap_listp = NEXT_BLKP(heap_listp);
    }

    /* Available block not found */
    return NULL;
}

static void place(void *bp, size_t asize)
{
    int block_size = GET_SIZE(HDRP(bp));
    int left_size = block_size - asize;

    /* Place block */
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));

    /* Place left block */
    if (left_size) {
        PUT(HDRP(NEXT_BLKP(bp)), PACK(left_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(left_size, 0));
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *heap_listp = NULL;

    /* Create the initial heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);                         /* Alignment padding */
    PUT(heap_listp + (WSIZE), PACK(DSIZE, 1));  /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));/* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));    /* Epilogue header */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
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
    size_t extendsize;  /* Amount to extend a heap if no fit */
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
    size_t copySize, oldSize, nextSize;
    size_t asize;

    /* If ptr == NULL */
    if (ptr == NULL)
        return mm_malloc(size);

    /* If size == 0 */
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    oldSize = GET_SIZE(HDRP(ptr));

    if (asize < oldSize) {           /* New block size is smaller than old block */
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldSize - asize, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(oldSize - asize, 0));
        return ptr;
    } else if (asize == oldSize) {   /* New block size is equal to old block */
        return ptr;
    } else {                        /* New block size is bigger than old block */
        nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        if (nextSize + oldSize >= asize) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            if (nextSize + oldSize > asize) {
                PUT(HDRP(NEXT_BLKP(ptr)), PACK(nextSize + oldSize - asize, 0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(nextSize + oldSize - asize, 0));
            }
            return ptr;
        } else {
            newptr = mm_malloc(size);
            if (newptr == NULL)
              return NULL;
            copySize = oldSize - DSIZE;
            if (size < copySize)
              copySize = size;
            memcpy(newptr, ptr, copySize);
            mm_free(ptr);
            return newptr;
        }
    }
}

/*
 * mm_check - Heap consistency checker
 */
int mm_check(void)
{
    return 1;
}