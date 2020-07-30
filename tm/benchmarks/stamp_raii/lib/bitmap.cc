/* =============================================================================
 *
 * bitmap.c
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
#include <string.h>
#include "bitmap.h"
#include "../def.h"
#include "../../../common/tm_api.h"
#include "types.h"
#include "utility.h"


#define NUM_BIT_PER_BYTE (8L)
#define NUM_BIT_PER_WORD (sizeof(unsigned long) * NUM_BIT_PER_BYTE)


/* =============================================================================
 * bitmap_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE bitmap_t*
bitmap_alloc (long numBit)
{
    bitmap_t* bitmapPtr;

    bitmapPtr = (bitmap_t*)malloc(sizeof(bitmap_t));
    if (bitmapPtr == NULL) {
        return NULL;
    }

    bitmapPtr->numBit = numBit;
    long numWord = DIVIDE_AND_ROUND_UP(numBit, NUM_BIT_PER_WORD);
    bitmapPtr->numWord = numWord;

    bitmapPtr->bits = (unsigned long*)malloc(numWord * sizeof(unsigned long));
    if (bitmapPtr->bits == NULL) {
        return NULL;
    }
    memset(bitmapPtr->bits, 0, (numWord * sizeof(unsigned long)));

    return bitmapPtr;
}


/* =============================================================================
 * Pbitmap_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE bitmap_t*
Pbitmap_alloc (long numBit)
{
    bitmap_t* bitmapPtr;

    bitmapPtr = (bitmap_t*)malloc(sizeof(bitmap_t));
    if (bitmapPtr == NULL) {
        return NULL;
    }

    bitmapPtr->numBit = numBit;
    long numWord = DIVIDE_AND_ROUND_UP(numBit, NUM_BIT_PER_WORD);
    bitmapPtr->numWord = numWord;

    //    bitmapPtr->bits = (unsigned long*)malloc(numWord * sizeof(unsigned long));
    bitmapPtr->bits = (unsigned long*)aligned_alloc(32,numWord * sizeof(unsigned long));
    if (bitmapPtr->bits == NULL) {
        free(bitmapPtr);
        return NULL;
    }
    memset(bitmapPtr->bits, 0, (numWord * sizeof(unsigned long)));

    return bitmapPtr;
}


/* =============================================================================
 * bitmap_free
 * =============================================================================
 */
TX_SAFE void
bitmap_free (bitmap_t* bitmapPtr)
{
    free(bitmapPtr->bits);
    free(bitmapPtr);
}


/* =============================================================================
 * Pbitmap_free
 * =============================================================================
 */
TX_SAFE void
Pbitmap_free (bitmap_t* bitmapPtr)
{
    free(bitmapPtr->bits);
    free(bitmapPtr);
}



/* =============================================================================
 * bitmap_set
 * -- Sets ith bit to 1
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TX_SAFE bool
bitmap_set (bitmap_t* bitmapPtr, long i)
{
    if ((i < 0) || (i >= bitmapPtr->numBit)) {
        return FALSE;
    }

    bitmapPtr->bits[i/NUM_BIT_PER_WORD] |= (1UL << (i % NUM_BIT_PER_WORD));

    return TRUE;
}


/* =============================================================================
 * bitmap_clear
 * -- Clears ith bit to 0
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TX_SAFE bool
bitmap_clear (bitmap_t* bitmapPtr, long i)
{
    if ((i < 0) || (i >= bitmapPtr->numBit)) {
        return FALSE;
    }

    bitmapPtr->bits[i/NUM_BIT_PER_WORD] &= ~(1UL << (i % NUM_BIT_PER_WORD));

    return TRUE;
}


/* =============================================================================
 * bitmap_clearAll
 * -- Clears all bit to 0
 * =============================================================================
 */
TX_SAFE void
bitmap_clearAll (bitmap_t* bitmapPtr)
{
    memset(bitmapPtr->bits, 0, (bitmapPtr->numWord * sizeof(unsigned long)));
}


/* =============================================================================
 * bitmap_isClear
 * -- Returns TRUE if ith bit is clear, else FALSE
 * =============================================================================
 */
TX_SAFE bool
bitmap_isClear (bitmap_t* bitmapPtr, long i)
{
    if ((i >= 0) && (i < bitmapPtr->numBit) &&
        !(bitmapPtr->bits[i/NUM_BIT_PER_WORD] & (1UL << (i % NUM_BIT_PER_WORD)))) {
        return TRUE;
    }

    return FALSE;
}


/* =============================================================================
 * bitmap_isSet
 * -- Returns TRUE if ith bit is set, else FALSE
 * =============================================================================
 */
TX_SAFE bool
bitmap_isSet (bitmap_t* bitmapPtr, long i)
{
    if ((i >= 0) && (i < bitmapPtr->numBit) &&
        (bitmapPtr->bits[i/NUM_BIT_PER_WORD] & (1UL << (i % NUM_BIT_PER_WORD)))) {
        return TRUE;
    }

    return FALSE;
}


/* =============================================================================
 * bitmap_findClear
 * -- Returns index of first clear bit
 * -- If start index is negative, will start from beginning
 * -- If all bits are set, returns -1
 * =============================================================================
 */
TX_SAFE long
bitmap_findClear (bitmap_t* bitmapPtr, long startIndex)
{
    long i;
    long numBit = bitmapPtr->numBit;
    unsigned long* bits = bitmapPtr->bits;

#ifdef __GNUC__
    for (i = MAX(startIndex, 0); i < numBit; i++) {
#else
    for (i = MAX_nogcc(startIndex, 0); i < numBit; i++) {
#endif
        if (!(bits[i/NUM_BIT_PER_WORD] & (1UL << (i % NUM_BIT_PER_WORD)))) {
            return i;
        }
    }

    return -1;
}


/* =============================================================================
 * bitmap_findSet
 * -- Returns index of first set bit
 * -- If start index is negative, will start from beginning
 * -- If all bits are clear, returns -1
 * =============================================================================
 */
TX_SAFE long
bitmap_findSet (bitmap_t* bitmapPtr, long startIndex)
{
    long i;
    long numBit = bitmapPtr->numBit;
    unsigned long* bits = bitmapPtr->bits;

#ifdef __GNUC__
    for (i = MAX(startIndex, 0); i < numBit; i++) {
#else
    for (i = MAX_nogcc(startIndex, 0); i < numBit; i++) {
#endif
        if (bits[i/NUM_BIT_PER_WORD] & (1UL << (i % NUM_BIT_PER_WORD))) {
            return i;
        }
    }

    return -1;
}


/* =============================================================================
 * bitmap_getNumClear
 * =============================================================================
 */
TX_SAFE long
bitmap_getNumClear (bitmap_t* bitmapPtr)
{
    long numBit = bitmapPtr->numBit;

    return (numBit - bitmap_getNumSet(bitmapPtr));
}


/* =============================================================================
 * bitmap_getNumSet
 * =============================================================================
 */
TX_SAFE long
bitmap_getNumSet (bitmap_t* bitmapPtr)
{
    long i;
    long numBit = bitmapPtr->numBit;
    unsigned long* bits = bitmapPtr->bits;
    long count = 0;

    for (i = 0; i < numBit; i++) {
        if (bits[i/NUM_BIT_PER_WORD] & (1UL << (i % NUM_BIT_PER_WORD))) {
            count++;
        }
    }

    return count;
}


/* =============================================================================
 * bitmap_copy
 * =============================================================================
 */
TX_SAFE void
bitmap_copy (bitmap_t* dstPtr, bitmap_t* srcPtr)
{
    assert(dstPtr->numBit == srcPtr->numBit);
    memcpy(dstPtr->bits, srcPtr->bits, (dstPtr->numWord * sizeof(unsigned long)));
}


/* =============================================================================
 * bitmap_toggleAll
 * =============================================================================
 */
TX_SAFE void
bitmap_toggleAll (bitmap_t* bitmapPtr)
{
    unsigned long* bits = bitmapPtr->bits;
    long numWord = bitmapPtr->numWord;
    long w;
    for (w = 0; w < numWord; w++) {
        bits[w] ^= (unsigned long)(-1L);
    }
}


/* =============================================================================
 * TEST_BITMAP
 * =============================================================================
 */
#ifdef TEST_BITMAP


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


int
main ()
{
    bitmap_t* bitmapPtr;
    long numBit = 320;
    long i;
    long j;

    srand(0);

    puts("Starting...");

    bitmapPtr = bitmap_alloc(numBit);

    assert(bitmapPtr->numBit == numBit);
#ifdef __GNUC__
    assert(bitmapPtr->numWord == (DIVIDE_AND_ROUND_UP(numBit, NUM_BIT_PER_WORD)));
#else
    assert(bitmapPtr->numWord == (DIVIDE_AND_ROUND_UP_nogcc(numBit, NUM_BIT_PER_WORD)));
#endif

    /* Check that initial is all clear */
    for (i = 0; i < numBit; i++) {
        assert(bitmap_isClear(bitmapPtr, i));
        assert(!bitmap_isSet(bitmapPtr, i));
    }
    assert(bitmap_getNumClear(bitmapPtr) == numBit);
    assert(bitmap_getNumSet(bitmapPtr) == 0);

    /* Check bounds */
    assert(!bitmap_clear(bitmapPtr, -1));
    assert(!bitmap_set(bitmapPtr, -1));
    assert(!bitmap_clear(bitmapPtr, numBit+1));
    assert(!bitmap_set(bitmapPtr, numBit+1));

    /* Set random bits */
    for (i = 0, j = 0; i < numBit; i+=(rand() % 5 + 1)) {
        assert(bitmap_set(bitmapPtr, i));
        assert(bitmap_set(bitmapPtr, i));
        assert(bitmap_clear(bitmapPtr, i));
        assert(bitmap_set(bitmapPtr, i));
        assert(bitmap_set(bitmapPtr, i));
        assert(bitmap_isSet(bitmapPtr, i));
        j++;
    }
    assert(bitmap_getNumClear(bitmapPtr) == (numBit - j));
    assert(bitmap_getNumSet(bitmapPtr) == j);

    /* Clear set bits */
    while ((i = bitmap_findSet(bitmapPtr, -1)) >= 0) {
        assert(bitmap_clear(bitmapPtr, i));
        i++;
    }
    assert(bitmap_getNumClear(bitmapPtr) == numBit);
    assert(bitmap_getNumSet(bitmapPtr) == 0);
    assert(bitmap_findSet(bitmapPtr, -1) == -1);

    /* Set all bits */
    i = -1;
    while ((i = bitmap_findClear(bitmapPtr, i)) >= 0) {
        assert(bitmap_set(bitmapPtr, i));
        i++;
    }
    assert(bitmap_getNumClear(bitmapPtr) == 0);
    assert(bitmap_getNumSet(bitmapPtr) == numBit);
    assert(bitmap_findClear(bitmapPtr, -1) == -1);

    /* Clear random bits */
    for (i = 0, j = 0; i < numBit; i+=(rand() % 5 + 1)) {
        assert(bitmap_clear(bitmapPtr, i));
        assert(bitmap_clear(bitmapPtr, i));
        assert(bitmap_set(bitmapPtr, i));
        assert(bitmap_clear(bitmapPtr, i));
        assert(bitmap_clear(bitmapPtr, i));
        assert(bitmap_isClear(bitmapPtr, i));
        j++;
    }
    assert(bitmap_getNumClear(bitmapPtr) == j);
    assert(bitmap_getNumSet(bitmapPtr) == (numBit - j));

    bitmap_free(bitmapPtr);

    puts("All tests passed.");

    return 0;
}


#endif /* TEST_BITMAP */


/* =============================================================================
 *
 * End of bitmap.c
 *
 * =============================================================================
 */
