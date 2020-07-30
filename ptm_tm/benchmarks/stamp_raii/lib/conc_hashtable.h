/* Copyright (c) IBM Corp. 2014. */
#ifndef CONC_HASHTABLE_H
#define CONC_HASHTABLE_H 1

#include "hashtable.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct conc_hashtable {
    char *segments;
    char *segmentsUnaligned;
    ulong_t (*hash)(const void*);
} conc_hashtable_t;

conc_hashtable_t*
conc_hashtable_alloc (long initNumBucket,
		      ulong_t (*hash)(const void*),
		      long (*comparePairs)(const pair_t*, const pair_t*),
		      long resizeRatio,
		      long growthFactor);

void
conc_hashtable_free (conc_hashtable_t* concHashtablePtr);

bool_t
conc_hashtable_containsKey (conc_hashtable_t* concHashtablePtr, void* keyPtr);

void*
conc_hashtable_find (conc_hashtable_t* concHashtablePtr, void* keyPtr);

bool_t
conc_hashtable_insert (conc_hashtable_t* concHashtablePtr, void* keyPtr, void* dataPtr);

bool_t
conc_hashtable_remove (conc_hashtable_t* concHashtablePtr, void* keyPtr);



conc_hashtable_t*
TMconc_hashtable_alloc (TM_ARGDECL
			long initNumBucket,
			ulong_t (*hash)(const void*),
			long (*comparePairs)(const pair_t*, const pair_t*),
			long resizeRatio,
			long growthFactor);

void
TMconc_hashtable_free (TM_ARGDECL  conc_hashtable_t* concHashtablePtr);

bool_t
TMconc_hashtable_containsKey (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr);

void*
TMconc_hashtable_find (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr);

bool_t
TMconc_hashtable_insert (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr, void* dataPtr);

bool_t
TMconc_hashtable_remove (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr);



#define TMCONC_HASHTABLE_ALLOC(i, h, c, r, g)  TMconc_hashtable_alloc(TM_ARG  i, h, c, r, g)
#define TMCONC_HASHTABLE_FREE(ht)              TMconc_hashtable_free(TM_ARG  ht)
#define TMCONC_HASHTABLE_CONTAINSKEY(ht, k)    TMconc_hashtable_containsKey(TM_ARG ht, k)
#define TMCONC_HASHTABLE_FIND(ht, k)           TMconc_hashtable_find(TM_ARG ht, k)
#define TMCONC_HASHTABLE_INSERT(ht, k, d)      TMconc_hashtable_insert(TM_ARG ht, k, d)
#define TMCONC_HASHTABLE_REMOVE(ht, k)         TMconc_hashtable_remove(TM_ARG ht, k)

#ifdef __cplusplus
}
#endif


#endif /* CONC_HASHTABLE_H */
