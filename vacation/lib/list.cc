/* =============================================================================
 *
 * list.c
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


#include <stdlib.h>
#include <assert.h>
#include "list.h"
#include "types.h"
#include "../def.h"
#include "../../include/tm.h"


/* =============================================================================
 * DECLARATION OF TM_CALLABLE FUNCTIONS
 * =============================================================================
 */


TM_CALLABLE
list_node_t*
findPrevious (TM_ARGDECL  list_t* listPtr, void* dataPtr);

TM_CALLABLE
void
freeList (TM_ARGDECL  list_node_t* nodePtr);

TM_CALLABLE
void
list_free (TM_ARGDECL  list_t* listPtr);

/* =============================================================================
 * compareDataPtrAddresses
 * -- Default compare function
 * =============================================================================
 */
TX_SAFE long
compareDataPtrAddresses (const void* a, const void* b)
{
    return ((long)a - (long)b);
}


/* =============================================================================
 * list_iter_reset
 * =============================================================================
 */
TX_SAFE void
list_iter_reset (list_iter_t* itPtr, list_t* listPtr)
{
    *itPtr = &(listPtr->head);
}


/* =============================================================================
 * list_iter_hasNext
 * =============================================================================
 */
TX_SAFE bool
list_iter_hasNext (list_iter_t* itPtr, list_t* listPtr)
{
    return (((*itPtr)->nextPtr != NULL) ? TRUE : FALSE);
}

/* =============================================================================
 * list_iter_next
 * =============================================================================
 */
TX_SAFE void*
list_iter_next (list_iter_t* itPtr, list_t* listPtr)
{
    *itPtr = (*itPtr)->nextPtr;

    return (*itPtr)->dataPtr;
}


/* =============================================================================
 * allocNode
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE list_node_t*
allocNode (void* dataPtr)
{
  //  list_node_t* nodePtr = (list_node_t*)malloc(sizeof(list_node_t));
  list_node_t* nodePtr = (list_node_t*)aligned_alloc(32,sizeof(list_node_t));
  if (nodePtr == NULL) {
        return NULL;
    }

    nodePtr->dataPtr = dataPtr;
    nodePtr->nextPtr = NULL;

    return nodePtr;
}


/* =============================================================================
 * PallocNode
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE list_node_t*
PallocNode (void* dataPtr)
{
  //  list_node_t* nodePtr = (list_node_t*)malloc(sizeof(list_node_t));
  list_node_t* nodePtr = (list_node_t*)aligned_alloc(32,sizeof(list_node_t));
    if (nodePtr == NULL) {
        return NULL;
    }

    nodePtr->dataPtr = dataPtr;
    nodePtr->nextPtr = NULL;

    return nodePtr;
}


/* =============================================================================
 * list_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE list_t*
list_alloc (long (*compare)(const void*, const void*))
{
  //  list_t* listPtr = (list_t*)malloc(sizeof(list_t));
  list_t* listPtr = (list_t*)aligned_alloc(32,sizeof(list_t));
    if (listPtr == NULL) {
        return NULL;
    }

    listPtr->head.dataPtr = NULL;
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;

    if (compare == NULL) {
        listPtr->compare = &compareDataPtrAddresses; /* default */
    } else {
        listPtr->compare = compare;
    }

    return listPtr;
}


/* =============================================================================
 * Plist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TX_SAFE list_t*
Plist_alloc (long (*compare)(const void*, const void*))
{
  //  list_t* listPtr = (list_t*)malloc(sizeof(list_t));
  list_t* listPtr = (list_t*)aligned_alloc(32,sizeof(list_t));
    if (listPtr == NULL) {
        return NULL;
    }

    listPtr->head.dataPtr = NULL;
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;

    if (compare == NULL) {
        listPtr->compare = &compareDataPtrAddresses; /* default */
    } else {
        listPtr->compare = compare;
    }

    return listPtr;
}


/* =============================================================================
 * freeNode
 * =============================================================================
 */
TX_SAFE void
freeNode (list_node_t* nodePtr)
{
  free(nodePtr);
}


/* =============================================================================
 * PfreeNode
 * =============================================================================
 */
TX_SAFE void
PfreeNode (list_node_t* nodePtr)
{
  free(nodePtr);
}


/* =============================================================================
 * freeList
 * =============================================================================
 */
TX_SAFE void
freeList (list_node_t* nodePtr)
{
    if (nodePtr != NULL) {
        freeList(nodePtr->nextPtr);
        freeNode(nodePtr);
    }
}


/* =============================================================================
 * PfreeList
 * =============================================================================
 */
TX_SAFE void
PfreeList (list_node_t* nodePtr)
{
    if (nodePtr != NULL) {
        PfreeList(nodePtr->nextPtr);
        PfreeNode(nodePtr);
    }
}


/* =============================================================================
 * list_free
 * =============================================================================
 */
TX_SAFE void
list_free (list_t* listPtr)
{
    freeList(listPtr->head.nextPtr);
    free(listPtr);
}


/* =============================================================================
 * Plist_free
 * =============================================================================
 */
TX_SAFE void
Plist_free (list_t* listPtr)
{
    PfreeList(listPtr->head.nextPtr);
    free(listPtr);
}


/* =============================================================================
 * list_isEmpty
 * -- Return TRUE if list is empty, else FALSE
 * =============================================================================
 */
TX_SAFE bool
list_isEmpty (list_t* listPtr)
{
    return (listPtr->head.nextPtr == NULL);
}

/* =============================================================================
 * list_getSize
 * -- Returns the size of the list
 * =============================================================================
 */
TX_SAFE long
list_getSize (list_t* listPtr)
{
    return listPtr->size;
}


/* =============================================================================
 * findPrevious
 * =============================================================================
 */
TX_SAFE list_node_t*
findPrevious (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr = &(listPtr->head);
    list_node_t* nodePtr = prevPtr->nextPtr;

    for (; nodePtr != NULL; nodePtr = nodePtr->nextPtr) {
        if (listPtr->compare(nodePtr->dataPtr, dataPtr) >= 0) {
            return prevPtr;
        }
        prevPtr = nodePtr;
    }

    return prevPtr;
}



/* =============================================================================
 * list_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
TX_SAFE void*
list_find (list_t* listPtr, void* dataPtr)
{
    list_node_t* nodePtr;
    list_node_t* prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;

    if ((nodePtr == NULL) ||
        (listPtr->compare(nodePtr->dataPtr, dataPtr) != 0)) {
        return NULL;
    }

    return (nodePtr->dataPtr);
}


/* =============================================================================
 * list_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
TX_SAFE bool
list_insert (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;
    list_node_t* currPtr;

    prevPtr = findPrevious(listPtr, dataPtr);
    currPtr = prevPtr->nextPtr;

#ifdef LIST_NO_DUPLICATES
    if ((currPtr != NULL) &&
        listPtr->compare(currPtr->dataPtr, dataPtr) == 0) {
        return FALSE;
    }
#endif

    nodePtr = allocNode(dataPtr);
    if (nodePtr == NULL) {
        return FALSE;
    }

    nodePtr->nextPtr = currPtr;
    prevPtr->nextPtr = nodePtr;
    listPtr->size++;

    return TRUE;
}


/* =============================================================================
 * Plist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
TX_SAFE bool
Plist_insert (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;
    list_node_t* currPtr;

    prevPtr = findPrevious(listPtr, dataPtr);
    currPtr = prevPtr->nextPtr;

#ifdef LIST_NO_DUPLICATES
    if ((currPtr != NULL) &&
        listPtr->compare(currPtr->dataPtr, dataPtr) == 0) {
        return FALSE;
    }
#endif

    nodePtr = PallocNode(dataPtr);
    if (nodePtr == NULL) {
        return FALSE;
    }

    nodePtr->nextPtr = currPtr;
    prevPtr->nextPtr = nodePtr;
    listPtr->size++;

    return TRUE;
}


/* =============================================================================
 * list_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
TX_SAFE bool
list_remove (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;

    prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;
    if ((nodePtr != NULL) &&
        (listPtr->compare(nodePtr->dataPtr, dataPtr) == 0))
    {
        prevPtr->nextPtr = nodePtr->nextPtr;
        nodePtr->nextPtr = NULL;
        freeNode(nodePtr);
        listPtr->size--;
        assert(listPtr->size >= 0);
        return TRUE;
    }

    return FALSE;
}


/* =============================================================================
 * Plist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
TX_SAFE bool
Plist_remove (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;

    prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;
    if ((nodePtr != NULL) &&
        (listPtr->compare(nodePtr->dataPtr, dataPtr) == 0))
    {
        prevPtr->nextPtr = nodePtr->nextPtr;
        nodePtr->nextPtr = NULL;
        PfreeNode(nodePtr);
        listPtr->size--;
        assert(listPtr->size >= 0);
        return TRUE;
    }

    return FALSE;
}


/* =============================================================================
 * list_clear
 * -- Removes all elements
 * =============================================================================
 */
TX_SAFE void
list_clear (list_t* listPtr)
{
    freeList(listPtr->head.nextPtr);
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;
}


/* =============================================================================
 * Plist_clear
 * -- Removes all elements
 * =============================================================================
 */
TX_SAFE void
Plist_clear (list_t* listPtr)
{
    PfreeList(listPtr->head.nextPtr);
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;
}


/* =============================================================================
 * TEST_LIST
 * =============================================================================
 */
#ifdef TEST_LIST


#include <assert.h>
#include <stdio.h>


static long
compare (const void* a, const void* b)
{
    return (*((const long*)a) - *((const long*)b));
}


static void
printList (list_t* listPtr)
{
    list_iter_t it;
    printf("[");
    list_iter_reset(&it, listPtr);
    while (list_iter_hasNext(&it, listPtr)) {
        printf("%li ", *((long*)(list_iter_next(&it, listPtr))));
    }
    puts("]");
}


static void
insertInt (list_t* listPtr, long* data)
{
    printf("Inserting: %li\n", *data);
    list_insert(listPtr, (void*)data);
    printList(listPtr);
}


static void
removeInt (list_t* listPtr, long* data)
{
    printf("Removing: %li\n", *data);
    list_remove(listPtr, (void*)data);
    printList(listPtr);
}


int
main ()
{
    list_t* listPtr;
#ifdef LIST_NO_DUPLICATES
    long data1[] = {3, 1, 4, 1, 5, -1};
#else
    long data1[] = {3, 1, 4, 5, -1};
#endif
    long data2[] = {3, 1, 4, 1, 5, -1};
    long i;

    puts("Starting...");

    puts("List sorted by values:");

    listPtr = list_alloc(&compare);

    for (i = 0; data1[i] >= 0; i++) {
        insertInt(listPtr, &data1[i]);
        assert(*((long*)list_find(listPtr, &data1[i])) == data1[i]);
    }

    for (i = 0; data1[i] >= 0; i++) {
        removeInt(listPtr, &data1[i]);
        assert(list_find(listPtr, &data1[i]) == NULL);
    }

    list_free(listPtr);

    puts("List sorted by addresses:");

    listPtr = list_alloc(NULL);

    for (i = 0; data2[i] >= 0; i++) {
        insertInt(listPtr, &data2[i]);
        assert(*((long*)list_find(listPtr, &data2[i])) == data2[i]);
    }

    for (i = 0; data2[i] >= 0; i++) {
        removeInt(listPtr, &data2[i]);
        assert(list_find(listPtr, &data2[i]) == NULL);
    }

    list_free(listPtr);

    puts("Done.");

    return 0;
}


#endif /* TEST_LIST */


/* =============================================================================
 *
 * End of list.c
 *
 * =============================================================================
 */

