/* =============================================================================
 *
 * table.h
 * -- Fixed-size hash table
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


#ifndef TABLE_H
#define TABLE_H 1


#include "list.h"
#include "types.h"


typedef struct table {
    list_t** buckets;
    long numBucket;
} table_t;


/* =============================================================================
 * table_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
table_t*
table_alloc (long numBucket, long (*compare)(const void*, const void*));


/* =============================================================================
 * table_insert
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
TX_SAFE bool
table_insert (table_t* tablePtr, unsigned long hash, void* dataPtr);


/* =============================================================================
 * TMtable_insert
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool
TMtable_insert (TM_ARGDECL  table_t* tablePtr, unsigned long hash, void* dataPtr);


/* =============================================================================
 * table_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool
table_remove (table_t* tablePtr, unsigned long hash, void* dataPtr);


/* =============================================================================
 * table_free
 * =============================================================================
 */
void
table_free (table_t* tablePtr);


#define TMTABLE_INSERT(t, h, d)         table_insert(TM_ARG  t, h, d)


#endif /* TABLE_H */


/* =============================================================================
 *
 * End of table.h
 *
 * =============================================================================
 */
