/* =============================================================================
 *
 * rbtree.h
 * -- Red-black balanced binary search tree
 *
 * =============================================================================
 *
 * Copyright (C) Sun Microsystems Inc., 2006.  All Rights Reserved.
 * Authors: Dave Dice, Nir Shavit, Ori Shalev.
 *
 * STM: Transactional Locking for Disjoint Access Parallelism
 *
 * Transactional Locking II,
 * Dave Dice, Ori Shalev, Nir Shavit
 * DISC 2006, Sept 2006, Stockholm, Sweden.
 *
 * =============================================================================
 *
 * Modified by Chi Cao Minh
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


#ifndef RBTREE_H
#define RBTREE_H 1


#include "../../../common/tm_api.h"
#include "../def.h"
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct rbtree rbtree_t;
typedef struct node* rbtree_iter_t;


/* =============================================================================
 * rbtree_verify
 * =============================================================================
 */
long
rbtree_verify (rbtree_t* s, long verbose);


/* =============================================================================
 * rbtree_alloc
 * =============================================================================
 */
rbtree_t*
rbtree_alloc (long (*compare)(const void*, const void*));


/* =============================================================================
 * TMrbtree_alloc
 * =============================================================================
 */
TM_CALLABLE  rbtree_t*
TMrbtree_alloc (TM_ARGDECL  long (*compare)(const void*, const void*));


/* =============================================================================
 * rbtree_free
 * =============================================================================
 */
void
rbtree_free (rbtree_t* r);


/* =============================================================================
 * TMrbtree_free
 * =============================================================================
 */
TM_CALLABLE void
TMrbtree_free (TM_ARGDECL  rbtree_t* r);


/* =============================================================================
 * rbtree_insert
 * -- Returns TRUE on success
 * =============================================================================
 */
bool
rbtree_insert (rbtree_t* r, void* key, void* val);


/* =============================================================================
 * TMrbtree_insert
 * -- Returns TRUE on success
 * =============================================================================
 */
TM_CALLABLE
bool
TMrbtree_insert (TM_ARGDECL  rbtree_t* r, void* key, void* val);


/* =============================================================================
 * rbtree_delete
 * =============================================================================
 */
bool
rbtree_delete (rbtree_t* r, void* key);


/* =============================================================================
 * TMrbtree_delete
 * =============================================================================
 */
TM_CALLABLE
bool
TMrbtree_delete (TM_ARGDECL  rbtree_t* r, void* key);


/* =============================================================================
 * rbtree_update
 * -- Return FALSE if had to insert node first
 * =============================================================================
 */
bool
rbtree_update (rbtree_t* r, void* key, void* val);


/* =============================================================================
 * TMrbtree_update
 * -- Return FALSE if had to insert node first
 * =============================================================================
 */
TM_CALLABLE
bool
TMrbtree_update (TM_ARGDECL  rbtree_t* r, void* key, void* val);


/* =============================================================================
 * rbtree_get
 * =============================================================================
 */
void*
rbtree_get (rbtree_t* r, void* key);


/* =============================================================================
 * TMrbtree_get
 * =============================================================================
 */
TM_CALLABLE
void*
TMrbtree_get (TM_ARGDECL  rbtree_t* r, void* key);


/* =============================================================================
 * rbtree_contains
 * =============================================================================
 */
bool
rbtree_contains (rbtree_t* r, void* key);


/* =============================================================================
 * TMrbtree_contains
 * =============================================================================
 */
TM_CALLABLE
bool
TMrbtree_contains (TM_ARGDECL  rbtree_t* r, void* key);


/* =============================================================================
 * rbtree_get_root_key
 * =============================================================================
 */
void*
rbtree_get_root_key (rbtree_t* r);


/* =============================================================================
 * TMrbtree_get_root_key
 * =============================================================================
 */
TM_CALLABLE
void*
TMrbtree_get_root_key (TM_ARGDECL  rbtree_t* r);


#ifdef RBTREE_SIZE_FIELD
/* =============================================================================
 * rbtree_get_size
 * =============================================================================
 */
long
rbtree_get_size (rbtree_t* r);


/* =============================================================================
 * TMrbtree_get_root_key
 * =============================================================================
 */
TM_CALLABLE
long
TMrbtree_get_size (TM_ARGDECL  rbtree_t* r);
#endif


/* Fixed by Odaira begin */ 
#define TMRBTREE_ALLOC(r)          TMrbtree_alloc(TM_ARG  r)
/*#define TMRBTREE_ALLOC()          TMrbtree_alloc(TM_ARG_ALONE)*/
/* Fixed by Odaira end */ 
#define TMRBTREE_FREE(r)          TMrbtree_free(TM_ARG  r)
#define TMRBTREE_INSERT(r, k, v)  TMrbtree_insert(TM_ARG  r, (void*)(k), (void*)(v))
#define TMRBTREE_DELETE(r, k)     TMrbtree_delete(TM_ARG  r, (void*)(k))
#define TMRBTREE_UPDATE(r, k, v)  TMrbtree_update(TM_ARG  r, (void*)(k), (void*)(v))
#define TMRBTREE_GET(r, k)        TMrbtree_get(TM_ARG  r, (void*)(k))
#define TMRBTREE_CONTAINS(r, k)   TMrbtree_contains(TM_ARG  r, (void*)(k))
#define TMRBTREE_GET_ROOT_KEY(r)  TMrbtree_get_root_key(TM_ARG  r)
#ifdef RBTREE_SIZE_FIELD
#define TMRBTREE_GET_SIZE(r)      TMrbtree_get_size(TM_ARG  r)
#endif


/* =============================================================================
 * rbtree_iter_reset
 * =============================================================================
 */
void
rbtree_iter_reset (rbtree_iter_t* itPtr, rbtree_t* r);


/* =============================================================================
 * TMrbtree_iter_reset
 * =============================================================================
 */
TM_CALLABLE
void
TMrbtree_iter_reset (TM_ARGDECL  rbtree_iter_t* itPtr, rbtree_t* r);


/* =============================================================================
 * rbtree_iter_hasNext
 * =============================================================================
 */
bool
rbtree_iter_hasNext (rbtree_iter_t* itPtr, rbtree_t* r);


/* =============================================================================
 * TMrbtree_iter_hasNext
 * =============================================================================
 */
TM_CALLABLE
bool
TMrbtree_iter_hasNext (TM_ARGDECL  rbtree_iter_t* itPtr, rbtree_t* r);


/* =============================================================================
 * rbtree_iter_next
 * =============================================================================
 */
void*
rbtree_iter_next (rbtree_iter_t* itPtr, rbtree_t* r);


/* =============================================================================
 * TMrbtree_iter_next
 * =============================================================================
 */
TM_CALLABLE
void*
TMrbtree_iter_next (TM_ARGDECL  rbtree_iter_t* itPtr, rbtree_t* r);


#define TMRBTREE_ITER_RESET(it, r)      TMrbtree_iter_reset(TM_ARG  it, r)
#define TMRBTREE_ITER_HASNEXT(it, r)    TMrbtree_iter_hasNext(TM_ARG  it, r)
#define TMRBTREE_ITER_NEXT(it, r)       TMrbtree_iter_next(TM_ARG  it, r)


#ifdef __cplusplus
}
#endif



#endif /* RBTREE_H */



/* =============================================================================
 *
 * End of rbtree.h
 *
 * =============================================================================
 */
