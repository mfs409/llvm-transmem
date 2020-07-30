/* =============================================================================
 *
 * stream.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "detector.h"
#include "dictionary.h"
#include "map.h"
#include "packet.h"
#include "queue.h"
#include <random>
#include "stream.h"
#include"../../../common/tm_api.h"
#include "../def.h"
#include "vector.h"


struct stream {
    long percentAttack;
    std::mt19937* randomPtr;
    vector_t* allocVectorPtr;
    queue_t* packetQueuePtr;
    MAP_T* attackMapPtr;
};


/* =============================================================================
 * stream_alloc
 * =============================================================================
 */
stream_t*
stream_alloc (long percentAttack)
{
    stream_t* streamPtr;

    streamPtr = (stream_t*)malloc(sizeof(stream_t));
    if (streamPtr) {
        assert(percentAttack >= 0 && percentAttack <= 100);
        streamPtr->percentAttack = percentAttack;
        streamPtr->randomPtr = new std::mt19937();
        assert(streamPtr->randomPtr);
        streamPtr->allocVectorPtr = vector_alloc(1);
        assert(streamPtr->allocVectorPtr);
        streamPtr->packetQueuePtr = queue_alloc(-1);
        assert(streamPtr->packetQueuePtr);
        streamPtr->attackMapPtr = MAP_ALLOC(NULL, NULL);
        assert(streamPtr->attackMapPtr);
    }

    return streamPtr;
}


/* =============================================================================
 * stream_free
 * =============================================================================
 */
void
stream_free (stream_t* streamPtr)
{
    vector_t* allocVectorPtr = streamPtr->allocVectorPtr;
    long a;
    long numAlloc = vector_getSize(allocVectorPtr);

    for (a = 0; a < numAlloc; a++) {
        char* str = (char*)vector_at(allocVectorPtr, a);
        free(str);
    }

    MAP_FREE(streamPtr->attackMapPtr);
    queue_free(streamPtr->packetQueuePtr);
    vector_free(streamPtr->allocVectorPtr);
    delete streamPtr->randomPtr;
    free(streamPtr);
}


/* =============================================================================
 * splitIntoPackets
 * -- Packets will be equal-size chunks except for last one, which will have
 *    all extra bytes
 * =============================================================================
 */
static void
splitIntoPackets (char* str,
                  long flowId,
                  std::mt19937* randomPtr,
                  vector_t* allocVectorPtr,
                  queue_t* packetQueuePtr)
{
    long numByte = strlen(str);
    long numPacket = randomPtr->operator()() % numByte + 1;

    long numDataByte = numByte / numPacket;

    long p;
    for (p = 0; p < (numPacket - 1); p++) {
        bool status;
        char* bytes = (char*)malloc(PACKET_HEADER_LENGTH + numDataByte);
        assert(bytes);
        status = vector_pushBack(allocVectorPtr, (void*)bytes);
        assert(status);
        packet_t* packetPtr = (packet_t*)bytes;
        packetPtr->flowId      = flowId;
        packetPtr->fragmentId  = p;
        packetPtr->numFragment = numPacket;
        packetPtr->length      = numDataByte;
        memcpy(packetPtr->data, (str + p * numDataByte), numDataByte);
        status = queue_push(packetQueuePtr, (void*)packetPtr);
        assert(status);
    }

    bool status;
    long lastNumDataByte = numDataByte + numByte % numPacket;
    char* bytes = (char*)malloc(PACKET_HEADER_LENGTH + lastNumDataByte);
    assert(bytes);
    status = vector_pushBack(allocVectorPtr, (void*)bytes);
    assert(status);
    packet_t* packetPtr = (packet_t*)bytes;
    packetPtr->flowId      = flowId;
    packetPtr->fragmentId  = p;
    packetPtr->numFragment = numPacket;
    packetPtr->length      = lastNumDataByte;
    memcpy(packetPtr->data, (str + p * numDataByte), lastNumDataByte);
    status = queue_push(packetQueuePtr, (void*)packetPtr);
    assert(status);
}


/* =============================================================================
 * stream_generate
 * -- Returns number of attacks generated
 * =============================================================================
 */
long
stream_generate (stream_t* streamPtr,
                 dictionary_t* dictionaryPtr,
                 long numFlow,
                 long seed,
                 long maxLength)
{
    long numAttack = 0;

    long      percentAttack  = streamPtr->percentAttack;
    std::mt19937* randomPtr      = streamPtr->randomPtr;
    vector_t* allocVectorPtr = streamPtr->allocVectorPtr;
    queue_t*  packetQueuePtr = streamPtr->packetQueuePtr;
    MAP_T*    attackMapPtr   = streamPtr->attackMapPtr;

    detector_t* detectorPtr = detector_alloc();
    assert(detectorPtr);
    detector_addPreprocessor(detectorPtr, &preprocessor_toLower);

    randomPtr->seed(seed);
    queue_clear(packetQueuePtr);

#ifdef __370__
    long range = '\x7e' - '\x20' + 1;
#else
    long range = '~' - ' ' + 1;
#endif
    assert(range > 0);

    long f;
    for (f = 1; f <= numFlow; f++) {
        char* str;
        if ((long)(randomPtr->operator()() % 100) < percentAttack) {
            long s = randomPtr->operator()() % global_numDefaultSignature;
            str = dictionary_get(dictionaryPtr, s);
            bool status =
                MAP_INSERT(attackMapPtr, (void*)f, (void*)str);
            assert(status);
            numAttack++;
        } else {
            /*
             * Create random string
             */
            long length = (randomPtr->operator()() % maxLength) + 1;
            str = (char*)malloc((length + 1) * sizeof(char));
            bool status = vector_pushBack(allocVectorPtr, (void*)str);
            assert(status);
            long l;
            for (l = 0; l < length; l++) {
#ifdef __370__
                str[l] = '\x20' + (char)(random_generate(randomPtr) % range);
#else
                str[l] = ' ' + (char)(randomPtr->operator()() % range);
#endif
            }
            str[l] = '\0';
	    char* str2 = (char*)malloc((length + 1) * sizeof(char));
            assert(str2);
            strcpy(str2, str);
            error_t error = detector_process(detectorPtr, str2); /* updates in-place */
            if (error == ERROR_SIGNATURE) {
                bool status = MAP_INSERT(attackMapPtr,
                                           (void*)f,
                                           (void*)str);
                assert(status);
                numAttack++;
            }
            free(str2);
        }
        splitIntoPackets(str, f, randomPtr, allocVectorPtr, packetQueuePtr);
    }

    queue_shuffle(packetQueuePtr, randomPtr);

    detector_free(detectorPtr);

    return numAttack;
}


/* =============================================================================
 * stream_getPacket
 * -- If none, returns NULL
 * =============================================================================
 */
char*
stream_getPacket (stream_t* streamPtr)
{
    return (char*)queue_pop(streamPtr->packetQueuePtr);
}


/* =============================================================================
 * TMstream_getPacket
 * -- If none, returns NULL
 * =============================================================================
 */
TM_CALLABLE char*
TMstream_getPacket (TM_ARGDECL stream_t* streamPtr)
{
    return (char*)TMQUEUE_POP(streamPtr->packetQueuePtr);
}


/* =============================================================================
 * stream_isAttack
 * =============================================================================
 */
bool
stream_isAttack (stream_t* streamPtr, long flowId)
{
    return MAP_CONTAINS(streamPtr->attackMapPtr, (void*)flowId);
}


/* #############################################################################
 * TEST_STREAM
 * #############################################################################
 */
#ifdef TEST_STREAM


#include <assert.h>
#include <stdio.h>


int
main ()
{
    long percentAttack = 10;
    long numFlow = 100;
    long seed = 0;
    long maxLength = 20;

    puts("Starting...");

    stream_t* streamPtr = stream_alloc(percentAttack);
    assert(streamPtr);

    dictionary_t* dictionaryPtr = dictionary_alloc();
    assert(dictionaryPtr);

    stream_generate(streamPtr, dictionaryPtr, numFlow, seed, maxLength);

    char* bytes;
    while ((bytes = stream_getPacket(streamPtr))) {
        packet_t* packetPtr = (packet_t*)bytes;
        long  flowId      = packetPtr->flowId;
        long  fragmentId  = packetPtr->fragmentId;
        long  numFragment = packetPtr->numFragment;
        long  length      = packetPtr->length;
        char* data        = packetPtr->data;
        long l;
        printf("flow=%2li frag=%2li num=%2li data=", flowId, fragmentId, numFragment);
        for (l = 0; l < length; l++) {
            printf("%c", data[l]);
        }
        puts("");
    }

    stream_free(streamPtr);

    puts("Done.");

    return 0;
}


#endif /* TEST_STREAM */


/* =============================================================================
 *
 * End of stream.c
 *
 * =============================================================================
 */
