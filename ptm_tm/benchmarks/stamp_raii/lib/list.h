/* =============================================================================
 *
 * list.h
 * -- Sorted singly linked list
 * -- Options: -DLIST_NO_DUPLICATES (default: allow duplicates)
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


#ifndef LIST_H
#define LIST_H 1

#include "../def.h"
#include "../../../common/tm_api.h"
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct list_node {
    void* dataPtr;
    struct list_node* nextPtr;
}  list_node_t;

typedef list_node_t* list_iter_t;

typedef struct list {
    list_node_t head;
    long (*compare)(const void*, const void*);   /* returns {-1,0,1}, 0 -> equal */
    long size;
} list_t;


/* =============================================================================
 * list_iter_reset
 * =============================================================================
 */
TX_SAFE void
list_iter_reset (list_iter_t* itPtr, list_t* listPtr);

/* =============================================================================
 * list_iter_hasNext
 * =============================================================================
 */
TX_SAFE bool
list_iter_hasNext (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * list_iter_next
 * =============================================================================
 */
TX_SAFE void*
list_iter_next (list_iter_t* itPtr, list_t* listPtr);

/* =============================================================================
 * list_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE list_t*
list_alloc (long (*compare)(const void*, const void*));


/* =============================================================================
 * Plist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE list_t*
Plist_alloc (long (*compare)(const void*, const void*));


/* =============================================================================
 * list_free
 * =============================================================================
 */
TX_SAFE void
list_free (list_t* listPtr);


/* =============================================================================
 * Plist_free
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE void
Plist_free (list_t* listPtr);

/* =============================================================================
 * list_isEmpty
 * -- Return TRUE if list is empty, else FALSE
 * =============================================================================
 */
TX_SAFE bool
list_isEmpty (list_t* listPtr);


/* =============================================================================
 * list_getSize
 * -- Returns size of list
 * =============================================================================
 */
TX_SAFE long
list_getSize (list_t* listPtr);

/* =============================================================================
 * list_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
TX_SAFE void*
list_find (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * list_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
TX_SAFE bool
list_insert (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * Plist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
TX_SAFE bool
Plist_insert (list_t* listPtr, void* dataPtr);

/* =============================================================================
 * list_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
TX_SAFE bool
list_remove (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * Plist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
TX_SAFE bool
Plist_remove (list_t* listPtr, void* dataPtr);

/* =============================================================================
 * list_clear
 * -- Removes all elements
 * =============================================================================
 */
TX_SAFE void
list_clear (list_t* listPtr);


/* =============================================================================
 * Plist_clear
 * -- Removes all elements
 * =============================================================================
 */
TX_SAFE void
Plist_clear (list_t* listPtr);


#define PLIST_ALLOC(cmp)                Plist_alloc(cmp)
#define PLIST_FREE(list)                Plist_free(list)
#define PLIST_GETSIZE(list)             list_getSize(list)
#define PLIST_INSERT(list, data)        Plist_insert(list, data)
#define PLIST_REMOVE(list, data)        Plist_remove(list, data)
#define PLIST_CLEAR(list)               Plist_clear(list)


#define TMLIST_ITER_RESET(it, list)     list_iter_reset(TM_ARG  it, list)
#define TMLIST_ITER_HASNEXT(it, list)   list_iter_hasNext(TM_ARG  it, list)
#define TMLIST_ITER_NEXT(it, list)      list_iter_next(TM_ARG  it, list)
#define TMLIST_ALLOC(cmp)               list_alloc(TM_ARG  cmp)
#define TMLIST_FREE(list)               list_free(TM_ARG  list)
#define TMLIST_GETSIZE(list)            list_getSize(TM_ARG  list)
#define TMLIST_ISEMPTY(list)            list_isEmpty(TM_ARG  list)
#define TMLIST_FIND(list, data)         list_find(TM_ARG  list, data)
#define TMLIST_INSERT(list, data)       list_insert(TM_ARG  list, data)
#define TMLIST_REMOVE(list, data)       list_remove(TM_ARG  list, data)


#ifdef __cplusplus
}
#endif


#endif /* LIST_H */


/* =============================================================================
 *
 * End of list.h
 *
 * =============================================================================
 */
