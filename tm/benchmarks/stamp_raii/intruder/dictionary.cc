/* =============================================================================
 *
 * signature.c
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


#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "dictionary.h"
#include"../../../common/tm_api.h"
#include "../def.h"
#include "types.h"
#include "vector.h"


const char* global_defaultSignatures[] = {
#ifdef __370__
    "\x61\x62\x6f\x75\x74",
    "\x61\x66\x74\x65\x72",
    "\x61\x6c\x6c",
    "\x61\x6c\x73\x6f",
    "\x61\x6e\x64",
    "\x61\x6e\x79",
    "\x62\x61\x63\x6b",
    "\x62\x65\x63\x61\x75\x73\x65",
    "\x62\x75\x74",
    "\x63\x61\x6e",
    "\x63\x6f\x6d\x65",
    "\x63\x6f\x75\x6c\x64",
    "\x64\x61\x79",
    "\x65\x76\x65\x6e",
    "\x66\x69\x72\x73\x74",
    "\x66\x6f\x72",
    "\x66\x72\x6f\x6d",
    "\x67\x65\x74",
    "\x67\x69\x76\x65",
    "\x67\x6f\x6f\x64",
    "\x68\x61\x76\x65",
    "\x68\x69\x6d",
    "\x68\x6f\x77",
    "\x69\x6e\x74\x6f",
    "\x69\x74\x73",
    "\x6a\x75\x73\x74",
    "\x6b\x6e\x6f\x77",
    "\x6c\x69\x6b\x65",
    "\x6c\x6f\x6f\x6b",
    "\x6d\x61\x6b\x65",
    "\x6d\x6f\x73\x74",
    "\x6e\x65\x77",
    "\x6e\x6f\x74",
    "\x6e\x6f\x77",
    "\x6f\x6e\x65",
    "\x6f\x6e\x6c\x79",
    "\x6f\x74\x68\x65\x72",
    "\x6f\x75\x74",
    "\x6f\x76\x65\x72",
    "\x70\x65\x6f\x70\x6c\x65",
    "\x73\x61\x79",
    "\x73\x65\x65",
    "\x73\x68\x65",
    "\x73\x6f\x6d\x65",
    "\x74\x61\x6b\x65",
    "\x74\x68\x61\x6e",
    "\x74\x68\x61\x74",
    "\x74\x68\x65\x69\x72",
    "\x74\x68\x65\x6d",
    "\x74\x68\x65\x6e",
    "\x74\x68\x65\x72\x65",
    "\x74\x68\x65\x73\x65",
    "\x74\x68\x65\x79",
    "\x74\x68\x69\x6e\x6b",
    "\x74\x68\x69\x73",
    "\x74\x69\x6d\x65",
    "\x74\x77\x6f",
    "\x75\x73\x65",
    "\x77\x61\x6e\x74",
    "\x77\x61\x79",
    "\x77\x65\x6c\x6c",
    "\x77\x68\x61\x74",
    "\x77\x68\x65\x6e",
    "\x77\x68\x69\x63\x68",
    "\x77\x68\x6f",
    "\x77\x69\x6c\x6c",
    "\x77\x69\x74\x68",
    "\x77\x6f\x72\x6b",
    "\x77\x6f\x75\x6c\x64",
    "\x79\x65\x61\x72",
    "\x79\x6f\x75\x72"
#else
    "about",
    "after",
    "all",
    "also",
    "and",
    "any",
    "back",
    "because",
    "but",
    "can",
    "come",
    "could",
    "day",
    "even",
    "first",
    "for",
    "from",
    "get",
    "give",
    "good",
    "have",
    "him",
    "how",
    "into",
    "its",
    "just",
    "know",
    "like",
    "look",
    "make",
    "most",
    "new",
    "not",
    "now",
    "one",
    "only",
    "other",
    "out",
    "over",
    "people",
    "say",
    "see",
    "she",
    "some",
    "take",
    "than",
    "that",
    "their",
    "them",
    "then",
    "there",
    "these",
    "they",
    "think",
    "this",
    "time",
    "two",
    "use",
    "want",
    "way",
    "well",
    "what",
    "when",
    "which",
    "who",
    "will",
    "with",
    "work",
    "would",
    "year",
    "your"
#endif
};

const long global_numDefaultSignature =
    sizeof(global_defaultSignatures) / sizeof(global_defaultSignatures[0]);


/* =============================================================================
 * dictionary_alloc
 * =============================================================================
 */
dictionary_t*
dictionary_alloc ()
{
    dictionary_t* dictionaryPtr = vector_alloc(global_numDefaultSignature);

    if (dictionaryPtr) {
        long s;
        for (s = 0; s < global_numDefaultSignature; s++) {
            const char* sig = global_defaultSignatures[s];
            bool status = vector_pushBack(dictionaryPtr,
                                            (void*)sig);
            assert(status);
        }
    }

    return dictionaryPtr;
}


/* =============================================================================
 * Pdictionary_alloc
 * =============================================================================
 */
dictionary_t*
Pdictionary_alloc ()
{
    dictionary_t* dictionaryPtr = PVECTOR_ALLOC(global_numDefaultSignature);

    if (dictionaryPtr) {
        long s;
        for (s = 0; s < global_numDefaultSignature; s++) {
            const char* sig = global_defaultSignatures[s];
            bool status = PVECTOR_PUSHBACK(dictionaryPtr,
                                             (void*)sig);
            assert(status);
        }
    }

    return dictionaryPtr;
}


/* =============================================================================
 * dictionary_free
 * =============================================================================
 */
void
dictionary_free (dictionary_t* dictionaryPtr)
{
    vector_free(dictionaryPtr);
}


/* =============================================================================
 * Pdictionary_free
 * =============================================================================
 */
void
Pdictionary_free (dictionary_t* dictionaryPtr)
{
    PVECTOR_FREE(dictionaryPtr);
}


/* =============================================================================
 * dictionary_add
 * =============================================================================
 */
bool
dictionary_add (dictionary_t* dictionaryPtr, char* str)
{
    return vector_pushBack(dictionaryPtr, (void*)str);
}


/* =============================================================================
 * dictionary_get
 * =============================================================================
 */
char*
dictionary_get (dictionary_t* dictionaryPtr, long i)
{
    return (char*)vector_at(dictionaryPtr, i);
}


/* =============================================================================
 * dictionary_match
 * =============================================================================
 */
char*
dictionary_match (dictionary_t* dictionaryPtr, char* str)
{
    long s;
    long numSignature = vector_getSize(dictionaryPtr);

    for (s = 0; s < numSignature; s++) {
        char* sig = (char*)vector_at(dictionaryPtr, s);
        if (strstr(str, sig) != NULL) {
            return sig;
        }
    }

    return NULL;
}


/* #############################################################################
 * TEST_DICTIONARY
 * #############################################################################
 */
#ifdef TEST_DICTIONARY


#include <assert.h>
#include <stdio.h>


int
main ()
{
    puts("Starting...");

    dictionary_t* dictionaryPtr;

    dictionaryPtr = dictionary_alloc();
    assert(dictionaryPtr);

    assert(dictionary_add(dictionaryPtr, "test1"));
    char* sig = dictionary_match(dictionaryPtr, "test1");
    assert(strcmp(sig, "test1") == 0);
    sig = dictionary_match(dictionaryPtr, "test1s");
    assert(strcmp(sig, "test1") == 0);
    assert(!dictionary_match(dictionaryPtr, "test2"));

    long s;
    for (s = 0; s < global_numDefaultSignature; s++) {
        char* sig = dictionary_match(dictionaryPtr, global_defaultSignatures[s]);
        assert(strcmp(sig, global_defaultSignatures[s]) == 0);
    }

    puts("All tests passed.");

    return 0;
}


#endif /* TEST_DICTIONARY */


/* =============================================================================
 *
 * End of dictionary.c
 *
 * =============================================================================
 */
