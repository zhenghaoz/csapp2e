/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following team struct 
 */
team_t team = {
    "team-z",              /* Team name */

    "Zhang Zhenghao",     /* First member full name */
    "zhangzhenghao@zjut.edu.cn",  /* First member email address */

    "",                   /* Second member full name (leave blank if none) */
    ""                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i+=32)
    for (j = 0; j < dim; j++) {
        dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
        dst[RIDX(dim-1-j, i+1, dim)] = src[RIDX(i+1, j, dim)];
        dst[RIDX(dim-1-j, i+2, dim)] = src[RIDX(i+2, j, dim)];
        dst[RIDX(dim-1-j, i+3, dim)] = src[RIDX(i+3, j, dim)];
        dst[RIDX(dim-1-j, i+4, dim)] = src[RIDX(i+4, j, dim)];
        dst[RIDX(dim-1-j, i+5, dim)] = src[RIDX(i+5, j, dim)];
        dst[RIDX(dim-1-j, i+6, dim)] = src[RIDX(i+6, j, dim)];
        dst[RIDX(dim-1-j, i+7, dim)] = src[RIDX(i+7, j, dim)];
        dst[RIDX(dim-1-j, i+8, dim)] = src[RIDX(i+8, j, dim)];
        dst[RIDX(dim-1-j, i+9, dim)] = src[RIDX(i+9, j, dim)];
        dst[RIDX(dim-1-j, i+10, dim)] = src[RIDX(i+10, j, dim)];
        dst[RIDX(dim-1-j, i+11, dim)] = src[RIDX(i+11, j, dim)];
        dst[RIDX(dim-1-j, i+12, dim)] = src[RIDX(i+12, j, dim)];
        dst[RIDX(dim-1-j, i+13, dim)] = src[RIDX(i+13, j, dim)];
        dst[RIDX(dim-1-j, i+14, dim)] = src[RIDX(i+14, j, dim)];
        dst[RIDX(dim-1-j, i+15, dim)] = src[RIDX(i+15, j, dim)];
        dst[RIDX(dim-1-j, i+16, dim)] = src[RIDX(i+16, j, dim)];
        dst[RIDX(dim-1-j, i+17, dim)] = src[RIDX(i+17, j, dim)];
        dst[RIDX(dim-1-j, i+18, dim)] = src[RIDX(i+18, j, dim)];
        dst[RIDX(dim-1-j, i+19, dim)] = src[RIDX(i+19, j, dim)];
        dst[RIDX(dim-1-j, i+20, dim)] = src[RIDX(i+20, j, dim)];
        dst[RIDX(dim-1-j, i+21, dim)] = src[RIDX(i+21, j, dim)];
        dst[RIDX(dim-1-j, i+22, dim)] = src[RIDX(i+22, j, dim)];
        dst[RIDX(dim-1-j, i+23, dim)] = src[RIDX(i+23, j, dim)];
        dst[RIDX(dim-1-j, i+24, dim)] = src[RIDX(i+24, j, dim)];
        dst[RIDX(dim-1-j, i+25, dim)] = src[RIDX(i+25, j, dim)];
        dst[RIDX(dim-1-j, i+26, dim)] = src[RIDX(i+26, j, dim)];
        dst[RIDX(dim-1-j, i+27, dim)] = src[RIDX(i+27, j, dim)];
        dst[RIDX(dim-1-j, i+28, dim)] = src[RIDX(i+28, j, dim)];
        dst[RIDX(dim-1-j, i+29, dim)] = src[RIDX(i+29, j, dim)];
        dst[RIDX(dim-1-j, i+30, dim)] = src[RIDX(i+30, j, dim)];
        dst[RIDX(dim-1-j, i+31, dim)] = src[RIDX(i+31, j, dim)];
    }
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() 
{
    add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
    /* ... Register additional test functions here */
}


/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
    int red;
    int green;
    int blue;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->num++;
    return;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned short) (sum.red/sum.num);
    current_pixel->green = (unsigned short) (sum.green/sum.num);
    current_pixel->blue = (unsigned short) (sum.blue/sum.num);
    return;
}

/* 
 * avg - Returns averaged pixel value at (i,j) 
 */
static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
	    accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth 
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

/*
 * smooth - Your current working version of smooth. 
 * IMPORTANT: This is the version you will be graded on
 */
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    pixel_sum col1, col2, col3;
    for (i = 0; i < dim; i++) {
        col2.red = col2.green = col2.blue = col2.num = 0;
        col3.red = col3.green = col3.blue = col3.num = 0;
        if (i < dim-1) {
            col3.red += src[RIDX(i+1, 0, dim)].red;
            col3.green += src[RIDX(i+1, 0, dim)].green;
            col3.blue += src[RIDX(i+1, 0, dim)].blue;
            col3.num++;
        }
        col3.red += src[RIDX(i, 0, dim)].red;
        col3.green += src[RIDX(i, 0, dim)].green;
        col3.blue += src[RIDX(i, 0, dim)].blue;
        col3.num++;
        if (i > 0) {
            col3.red += src[RIDX(i-1, 0, dim)].red;
            col3.green += src[RIDX(i-1, 0, dim)].green;
            col3.blue += src[RIDX(i-1, 0, dim)].blue;
            col3.num++;
        }
        for (j = 0; j < dim; j++) {
            col1 = col2;
            col2 = col3;
            col3.red = col3.green = col3.blue = col3.num = 0;
            if (j < dim-1) {
                col3.red += src[RIDX(i, j+1, dim)].red;
                col3.green += src[RIDX(i, j+1, dim)].green;
                col3.blue += src[RIDX(i, j+1, dim)].blue;
                col3.num++;
            }
            if (j < dim-1 && i > 0) {
                col3.red += src[RIDX(i-1, j+1, dim)].red;
                col3.green += src[RIDX(i-1, j+1, dim)].green;
                col3.blue += src[RIDX(i-1, j+1, dim)].blue;
                col3.num++;
            }
            if (j < dim-1 && i < dim-1) {
                col3.red += src[RIDX(i+1, j+1, dim)].red;
                col3.green += src[RIDX(i+1, j+1, dim)].green;
                col3.blue += src[RIDX(i+1, j+1, dim)].blue;
                col3.num++;
            }
            dst[RIDX(i, j, dim)].red = (unsigned short) ((col1.red + col2.red + col3.red)/(col1.num + col2.num + col3.num));
            dst[RIDX(i, j, dim)].green = (unsigned short) ((col1.green + col2.green + col3.green)/(col1.num + col2.num + col3.num));
            dst[RIDX(i, j, dim)].blue = (unsigned short) ((col1.blue + col2.blue + col3.blue)/(col1.num + col2.num + col3.num));
        }
    }
}


/********************************************************************* 
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_smooth_functions() {
    add_smooth_function(&naive_smooth, naive_smooth_descr);
    add_smooth_function(&smooth, smooth_descr);
    /* ... Register additional test functions here */
}

