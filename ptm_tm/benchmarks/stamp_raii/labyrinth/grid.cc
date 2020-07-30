/* =============================================================================
 *
 * grid.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */
/* Copyright (c) IBM Corp. 2014. */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "coordinate.h"
#include "grid.h"
#include"../../../common/tm_api.h"
#include "../def.h"
#include "types.h"
#include "vector.h"


#if defined( __370__)
const unsigned long CACHE_LINE_SIZE = 256UL;
#elif defined(__bgq__)
const unsigned long CACHE_LINE_SIZE = 128UL;
#elif defined(__PPC__) || defined(_ARCH_PPC)
const unsigned long CACHE_LINE_SIZE = 128UL;
#elif defined(__x86_64__)
const unsigned long CACHE_LINE_SIZE = 64UL;
#else
const unsigned long CACHE_LINE_SIZE = 32UL;
#endif


/* =============================================================================
 * grid_alloc
 * =============================================================================
 */
grid_t*
grid_alloc (long width, long height, long depth)
{
    grid_t* gridPtr;

    gridPtr = (grid_t*)malloc(sizeof(grid_t));
    if (gridPtr) {
        gridPtr->width  = width;
        gridPtr->height = height;
        gridPtr->depth  = depth;
        long n = width * height * depth;
        long* points_unaligned = (long*)malloc(n * sizeof(long) + CACHE_LINE_SIZE);
        assert(points_unaligned);
        gridPtr->points_unaligned = points_unaligned;
        gridPtr->points = (long*)((char*)(((unsigned long)points_unaligned
                                          & ~(CACHE_LINE_SIZE-1)))
                                  + CACHE_LINE_SIZE);
        memset(gridPtr->points, GRID_POINT_EMPTY, (n * sizeof(long)));
    }

    return gridPtr;
}


/* =============================================================================
 * Pgrid_alloc
 * =============================================================================
 */
TM_CALLABLE grid_t*
Pgrid_alloc (long width, long height, long depth)
{
    grid_t* gridPtr;

    gridPtr = (grid_t*)P_MALLOC(sizeof(grid_t));
    if (gridPtr) {
        gridPtr->width  = width;
        gridPtr->height = height;
        gridPtr->depth  = depth;
        long n = width * height * depth;
        long* points_unaligned = (long*)P_MALLOC(n * sizeof(long) + CACHE_LINE_SIZE);
        assert(points_unaligned);
        gridPtr->points_unaligned = points_unaligned;
        gridPtr->points = (long*)((char*)(((unsigned long)points_unaligned
                                          & ~(CACHE_LINE_SIZE-1)))
                                  + CACHE_LINE_SIZE);
        memset(gridPtr->points, GRID_POINT_EMPTY, (n * sizeof(long)));
    }

    return gridPtr;
}


/* =============================================================================
 * grid_free
 * =============================================================================
 */
void
grid_free (grid_t* gridPtr)
{
    free(gridPtr->points_unaligned);
    free(gridPtr);
}


/* =============================================================================
 * TMgrid_free
 * =============================================================================
 */
TM_CALLABLE void
Pgrid_free (grid_t* gridPtr)
{
    P_FREE(gridPtr->points_unaligned);
    P_FREE(gridPtr);
}


/* =============================================================================
 * grid_copy
 * =============================================================================
 */
TM_CALLABLE void
grid_copy (grid_t* dstGridPtr, grid_t* srcGridPtr)
{
    assert(srcGridPtr->width  == dstGridPtr->width);
    assert(srcGridPtr->height == dstGridPtr->height);
    assert(srcGridPtr->depth  == dstGridPtr->depth);

    long n = srcGridPtr->width * srcGridPtr->height * srcGridPtr->depth;
    memcpy(dstGridPtr->points, srcGridPtr->points, (n * sizeof(long)));

#ifdef USE_EARLY_RELEASE
    long* srcPoints = srcGridPtr->points;
    long i;
    long i_step = (CACHE_LINE_SIZE / sizeof(srcPoints[0]));
    for (i = 0; i < n; i+=i_step) {
        /* releases entire line [wer] means nothing in gcctm */
       // TM_EARLY_RELEASE(srcPoints[i]); /* releases entire line */
    }
#endif
}


/* =============================================================================
 * grid_isPointValid
 * =============================================================================
 */
TM_CALLABLE bool
grid_isPointValid (grid_t* gridPtr, long x, long y, long z)
{
    if (x < 0 || x >= gridPtr->width  ||
        y < 0 || y >= gridPtr->height ||
        z < 0 || z >= gridPtr->depth)
    {
        return FALSE;
    }

    return TRUE;
}


/* =============================================================================
 * grid_getPointRef
 * =============================================================================
 */
TM_CALLABLE long*
grid_getPointRef (grid_t* gridPtr, long x, long y, long z)
{
    return &(gridPtr->points[(z * gridPtr->height + y) * gridPtr->width + x]);
}


/* =============================================================================
 * grid_getPointIndices
 * =============================================================================
 */
TM_CALLABLE void
grid_getPointIndices (grid_t* gridPtr,
                      long* gridPointPtr, long* xPtr, long* yPtr, long* zPtr)
{
    long height = gridPtr->height;
    long width  = gridPtr->width;
    long area = height * width;
    long index3d = (gridPointPtr - gridPtr->points);
    (*zPtr) = index3d / area;
    long index2d = index3d % area;
    (*yPtr) = index2d / width;
    (*xPtr) = index2d % width;
}


/* =============================================================================
 * grid_getPoint
 * =============================================================================
 */
TM_CALLABLE long
grid_getPoint (grid_t* gridPtr, long x, long y, long z)
{
    return *grid_getPointRef(gridPtr, x, y, z);
}


/* =============================================================================
 * grid_isPointEmpty
 * =============================================================================
 */
TM_CALLABLE bool
grid_isPointEmpty (grid_t* gridPtr, long x, long y, long z)
{
    long value = grid_getPoint(gridPtr, x, y, z);
    return ((value == GRID_POINT_EMPTY) ? TRUE : FALSE);
}


/* =============================================================================
 * grid_isPointFull
 * =============================================================================
 */
TM_CALLABLE bool
grid_isPointFull (grid_t* gridPtr, long x, long y, long z)
{
    long value = grid_getPoint(gridPtr, x, y, z);
    return ((value == GRID_POINT_FULL) ? TRUE : FALSE);
}


/* =============================================================================
 * grid_setPoint
 * =============================================================================
 */
TM_CALLABLE void
grid_setPoint (grid_t* gridPtr, long x, long y, long z, long value)
{
    (*grid_getPointRef(gridPtr, x, y, z)) = value;
}


/* =============================================================================
 * grid_addPath
 * =============================================================================
 */
TM_CALLABLE void
grid_addPath (grid_t* gridPtr, vector_t* pointVectorPtr)
{
    long i;
    long n = vector_getSize(pointVectorPtr);

    for (i = 0; i < n; i++) {
        coordinate_t* coordinatePtr = (coordinate_t*)vector_at(pointVectorPtr, i);
        long x = coordinatePtr->x;
        long y = coordinatePtr->y;
        long z = coordinatePtr->z;
        grid_setPoint(gridPtr, x, y, z, GRID_POINT_FULL);
    }
}


/* =============================================================================
 * TMgrid_addPath
 * =============================================================================
 */
TM_CALLABLE void
TMgrid_addPath (TM_ARGDECL  grid_t* gridPtr, vector_t* pointVectorPtr)
{
    long i;
    long n = vector_getSize(pointVectorPtr);

    for (i = 1; i < (n-1); i++) {
        long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
        long value = (long)TM_SHARED_READ(*gridPointPtr);
        if (value != GRID_POINT_EMPTY) {
             return; //TM_RESTART();
        }
        TM_SHARED_WRITE(*gridPointPtr, GRID_POINT_FULL);
    }
}

TM_CALLABLE bool
TMgrid_addPath2 (vector_t* pointVectorPtr)
{
    long i;
    long n = vector_getSize(pointVectorPtr);
#if 0
    for (i = 1; i < (n-1); i++) {
        long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
        long value = (long)TM_SHARED_READ(*gridPointPtr);
        if (value != GRID_POINT_EMPTY) {
          _ITM_abortTransaction(2);
        }
        TM_SHARED_WRITE(*gridPointPtr, GRID_POINT_FULL);
    }
#endif
    //[wer210] a check loop and a write loop
    for (i = 1; i < (n-1); i++) {
      long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
      long value = *gridPointPtr;
      if (value != GRID_POINT_EMPTY) {
        return false;
      }
      //TM_SHARED_WRITE(*gridPointPtr, (long)GRID_POINT_FULL);
    }

    for (i = 1; i < (n-1); i++) {
      long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
      *gridPointPtr = (long)GRID_POINT_FULL;
    }
    return true;
}

/* =============================================================================
 * grid_print
 * =============================================================================
 */
void
grid_print (grid_t* gridPtr)
{
    long width  = gridPtr->width;
    long height = gridPtr->height;
    long depth  = gridPtr->depth;
    long z;

    for (z = 0; z < depth; z++) {
        printf("[z = %li]\n", z);
        long x;
        for (x = 0; x < width; x++) {
            long y;
            for (y = 0; y < height; y++) {
                printf("%4li", *grid_getPointRef(gridPtr, x, y, z));
            }
            puts("");
        }
        puts("");
    }
}


/* =============================================================================
 *
 * End of grid.c
 *
 * =============================================================================
 */
