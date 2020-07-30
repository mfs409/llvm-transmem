/* Copyright (c) IBM Corp. 2014. */
#include <stdlib.h>
#include <stdint.h>
#include "conc_hashtable.h"
#include "types.h"

#ifdef HAVE_CONFIG_H
# include "STAMP_config.h"
#endif

#if defined( __370__)
#define CACHE_LINE_SIZE 256
#elif defined(__bgq__)
#define CACHE_LINE_SIZE 128
#elif defined(__PPC__) || defined(_ARCH_PPC)
#define CACHE_LINE_SIZE 128
#elif defined(__x86_64__)
#define CACHE_LINE_SIZE 64
#else
#define CACHE_LINE_SIZE 32
#endif

#define ALIGNED_HASHTABLE_SIZE ((sizeof(hashtable_t) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))

#define NUM_SEGMENTS 31
#define GET_SEGMENT(seg, idx) ((hashtable_t *)((char *)(seg) + ALIGNED_HASHTABLE_SIZE * (idx)))

static unsigned long
hashKeyDefault(const void *a)
{
    return (unsigned long)a;
}

static long
comparePairsDefault (const pair_t* a, const pair_t* b)
{
    return ((long)(a->firstPtr) - (long)(b->firstPtr));
}

conc_hashtable_t*
conc_hashtable_alloc (long initNumBucket,
		      unsigned long (*hash)(const void*),
		      long (*comparePairs)(const pair_t*, const pair_t*),
		      long resizeRatio,
		      long growthFactor)
{
    conc_hashtable_t *concHashtablePtr;
    char *segmentsUnaligned;
    char *segments;
    int s;

    if (hash == NULL) {
	hash = hashKeyDefault;
    }
    if (comparePairs == NULL) {
	comparePairs = comparePairsDefault;
    }

    concHashtablePtr = (conc_hashtable_t *)malloc(sizeof(conc_hashtable_t));
    if (concHashtablePtr == NULL) {
	return NULL;
    }

    concHashtablePtr->hash = hash;

    segmentsUnaligned = (char *)malloc(sizeof(char) * ALIGNED_HASHTABLE_SIZE * NUM_SEGMENTS + CACHE_LINE_SIZE);
    if (segmentsUnaligned == NULL) {
	return NULL;
    }
    segments = (char *)(((uintptr_t)segmentsUnaligned + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));

    concHashtablePtr->segmentsUnaligned = segmentsUnaligned;
    concHashtablePtr->segments = segments;

    for (s = 0; s < NUM_SEGMENTS; s++) {
	if (hashtable_init(GET_SEGMENT(segments, s),
			   initNumBucket, hash, comparePairs, resizeRatio, growthFactor)) {
	    free(concHashtablePtr);
	    return NULL;
	}
    }

    return concHashtablePtr;
}

void
conc_hashtable_free (conc_hashtable_t* concHashtablePtr)
{
    int s;

    for (s = 0; s < NUM_SEGMENTS; s++) {
	hashtable_free_buckets(GET_SEGMENT(concHashtablePtr->segments, s));
    }

    free(concHashtablePtr->segmentsUnaligned);
    free(concHashtablePtr);
}

bool
conc_hashtable_containsKey (conc_hashtable_t* concHashtablePtr, void* keyPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return hashtable_containsKey(GET_SEGMENT(concHashtablePtr->segments, i), keyPtr);
}


void*
conc_hashtable_find (conc_hashtable_t* concHashtablePtr, void* keyPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return hashtable_find(GET_SEGMENT(concHashtablePtr->segments, i), keyPtr);
}

bool
conc_hashtable_insert (conc_hashtable_t* concHashtablePtr, void* keyPtr, void* dataPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return hashtable_insert(GET_SEGMENT(concHashtablePtr->segments, i), keyPtr, dataPtr);
}

bool
conc_hashtable_remove (conc_hashtable_t* concHashtablePtr, void* keyPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return hashtable_remove(GET_SEGMENT(concHashtablePtr->segments, i), keyPtr);
}



conc_hashtable_t*
TMconc_hashtable_alloc (TM_ARGDECL  long initNumBucket,
			unsigned long (*hash)(const void*),
			long (*comparePairs)(const pair_t*, const pair_t*),
			long resizeRatio,
			long growthFactor)
{
    conc_hashtable_t *concHashtablePtr;
    char *segmentsUnaligned;
    char *segments;
    int s;

    if (hash == NULL) {
	hash = hashKeyDefault;
    }
    if (comparePairs == NULL) {
	comparePairs = comparePairsDefault;
    }

    /* This will be freed only by TMconc_hashtable_free(),
       so it can be allocated by malloc(), not TM_MALLOC(). */
    //concHashtablePtr = (conc_hashtable_t *)malloc(sizeof(conc_hashtable_t));
    concHashtablePtr = (conc_hashtable_t *)aligned_alloc(32,sizeof(conc_hashtable_t));
    if (concHashtablePtr == NULL) {
	return NULL;
    }

    concHashtablePtr->hash = hash;

    /* This will be freed only by TMconc_hashtable_free(),
       so it can be allocated by malloc(), not TM_MALLOC(). */
    //    segmentsUnaligned = (char *)malloc(sizeof(char) * ALIGNED_HASHTABLE_SIZE * NUM_SEGMENTS + CACHE_LINE_SIZE);
    segmentsUnaligned = (char *)aligned_alloc(32,sizeof(char) * ALIGNED_HASHTABLE_SIZE * NUM_SEGMENTS + CACHE_LINE_SIZE);
    if (segmentsUnaligned == NULL) {
	return NULL;
    }
    segments = (char *)(((uintptr_t)segmentsUnaligned + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));

    concHashtablePtr->segmentsUnaligned = segmentsUnaligned;
    concHashtablePtr->segments = segments;

    for (s = 0; s < NUM_SEGMENTS; s++) {
	if (TMhashtable_init(TM_ARG  GET_SEGMENT(segments, s),
			     initNumBucket, hash, comparePairs, resizeRatio, growthFactor)) {
	    free(concHashtablePtr);
	    return NULL;
	}
    }

    return concHashtablePtr;
}

void
TMconc_hashtable_free (TM_ARGDECL  conc_hashtable_t* concHashtablePtr)
{
    int s;

    for (s = 0; s < NUM_SEGMENTS; s++) {
	TMhashtable_free_buckets(TM_ARG  GET_SEGMENT(concHashtablePtr->segments, s));
    }

    free(concHashtablePtr->segmentsUnaligned);
    free(concHashtablePtr);
}

bool
TMconc_hashtable_containsKey (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return TMhashtable_containsKey(TM_ARG  GET_SEGMENT(concHashtablePtr->segments, i), keyPtr);
}


void*
TMconc_hashtable_find (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return TMHASHTABLE_FIND(GET_SEGMENT(concHashtablePtr->segments, i), keyPtr);
}

bool
TMconc_hashtable_insert (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr, void* dataPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return TMHASHTABLE_INSERT(GET_SEGMENT(concHashtablePtr->segments, i), keyPtr, dataPtr);
}

bool
TMconc_hashtable_remove (TM_ARGDECL  conc_hashtable_t* concHashtablePtr, void* keyPtr)
{
    long i = concHashtablePtr->hash(keyPtr) % NUM_SEGMENTS;
    return TMHASHTABLE_REMOVE(GET_SEGMENT(concHashtablePtr->segments, i), keyPtr);
}
