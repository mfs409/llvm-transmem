/* =============================================================================
 *
 * labyrinth.c
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
#if defined(__370__) || defined(_AIX)
#include <unistd.h>  /* For getopt() */
#else
#include <getopt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "maze.h"
#include "router.h"
#include "thread.h"
#include "timer.h"
#include "types.h"
#if defined(__bgq__)
#include <stdint.h>
#include <spi/include/kernel/location.h>
#endif

enum param_types {
    PARAM_BENDCOST = (unsigned char)'b',
    PARAM_THREAD   = (unsigned char)'t',
    PARAM_XCOST    = (unsigned char)'x',
    PARAM_YCOST    = (unsigned char)'y',
    PARAM_ZCOST    = (unsigned char)'z',
};

enum param_defaults {
    PARAM_DEFAULT_BENDCOST = 1,
    PARAM_DEFAULT_THREAD   = 1,
    PARAM_DEFAULT_XCOST    = 1,
    PARAM_DEFAULT_YCOST    = 1,
    PARAM_DEFAULT_ZCOST    = 2,
};

bool global_doPrint = FALSE;
char* global_inputFile = NULL;
long global_params[256]; /* 256 = ascii limit */


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void
displayUsage (const char* appName)
{
    printf("Usage: %s [options]\n", appName);
    puts("\nOptions:                            (defaults)\n");
    printf("    b <INT>    [b]end cost          (%i)\n", PARAM_DEFAULT_BENDCOST);
    printf("    i <FILE>   [i]nput file name    (%s)\n", global_inputFile);
    printf("    p          [p]rint routed maze  (false)\n");
    printf("    t <UINT>   Number of [t]hreads  (%i)\n", PARAM_DEFAULT_THREAD);
    printf("    x <UINT>   [x] movement cost    (%i)\n", PARAM_DEFAULT_XCOST);
    printf("    y <UINT>   [y] movement cost    (%i)\n", PARAM_DEFAULT_YCOST);
    printf("    z <UINT>   [z] movement cost    (%i)\n", PARAM_DEFAULT_ZCOST);
    exit(1);
}


/* =============================================================================
 * setDefaultParams
 * =============================================================================
 */
static void
setDefaultParams ()
{
    global_params[PARAM_BENDCOST] = PARAM_DEFAULT_BENDCOST;
    global_params[PARAM_THREAD]   = PARAM_DEFAULT_THREAD;
    global_params[PARAM_XCOST]    = PARAM_DEFAULT_XCOST;
    global_params[PARAM_YCOST]    = PARAM_DEFAULT_YCOST;
    global_params[PARAM_ZCOST]    = PARAM_DEFAULT_ZCOST;
}


/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static void
parseArgs (long argc, char* const argv[])
{
    long i;
    long opt;

    opterr = 0;

    setDefaultParams();

    while ((opt = getopt(argc, argv, "b:i:pt:x:y:z:")) != -1) {
        switch (opt) {
            case 'b':
            case 't':
            case 'x':
            case 'y':
            case 'z':
                global_params[(unsigned char)opt] = atol(optarg);
                break;
            case 'i':
                global_inputFile = optarg;
                break;
            case 'p':
                global_doPrint = TRUE;
                break;
            case '?':
            default:
                opterr++;
                break;
        }
    }

    for (i = optind; i < argc; i++) {
        fprintf(stderr, "Non-option argument: %s\n", argv[i]);
        opterr++;
    }

    if (opterr) {
        displayUsage(argv[0]);
    }
}


/* =============================================================================
 * main
 * =============================================================================
 */
int main(int argc, char **argv)
{
    GOTO_REAL();

    /*
     * Initialization
     */
    parseArgs(argc, (char** const)argv);
#if defined(__bgq__)
    if (global_params[PARAM_THREAD] < 0) {
	uint32_t rank;
	long maxThreads;

	maxThreads = -global_params[PARAM_THREAD];
	rank = Kernel_GetRank();
	switch (rank) {
	case 0:
	    global_params[PARAM_THREAD] = 1;
	    break;
#if defined(GLOBAL_LOCK) || defined(HTM_IBM)
	case 1:
	    global_params[PARAM_THREAD] = 2;
	    break;
	case 2:
	    global_params[PARAM_THREAD] = 4;
	    break;
	case 3:
	    global_params[PARAM_THREAD] = 8;
	    break;
	case 4:
	    global_params[PARAM_THREAD] = 16;
	    break;
	case 5:
	    global_params[PARAM_THREAD] = 32;
	    break;
	case 6:
	    global_params[PARAM_THREAD] = 64;
	    break;
#endif
	default:
	    exit(0);
	    break;
	}
	if (maxThreads < global_params[PARAM_THREAD]) {
	    exit(0);
	}
    }
#endif
    long numThread = global_params[PARAM_THREAD];
    SIM_GET_NUM_CPU(numThread);
    TM_STARTUP(numThread);
    P_MEMORY_STARTUP(numThread);
    thread_startup(numThread);
    maze_t* mazePtr = maze_alloc();
    assert(mazePtr);
    long numPathToRoute = maze_read(mazePtr, global_inputFile);
    router_t* routerPtr = router_alloc(global_params[PARAM_XCOST],
                                       global_params[PARAM_YCOST],
                                       global_params[PARAM_ZCOST],
                                       global_params[PARAM_BENDCOST]);
    assert(routerPtr);
    list_t* pathVectorListPtr = list_alloc(NULL);
    assert(pathVectorListPtr);

    /*
     * Run transactions
     */
    router_solve_arg_t routerArg = {routerPtr, mazePtr, pathVectorListPtr};
    TIMER_T startTime;
    TIMER_READ(startTime);
    GOTO_SIM();
#ifdef OTM
#pragma omp parallel
    {
        router_solve((void *)&routerArg);
    }
#else
    thread_start(router_solve, (void*)&routerArg);
#endif
    GOTO_REAL();
    TIMER_T stopTime;
    TIMER_READ(stopTime);

    long numPathRouted = 0;
    list_iter_t it;
    list_iter_reset(&it, pathVectorListPtr);
    while (list_iter_hasNext(&it, pathVectorListPtr)) {
        vector_t* pathVectorPtr = (vector_t*)list_iter_next(&it, pathVectorListPtr);
        numPathRouted += vector_getSize(pathVectorPtr);
    }
    printf("Paths routed    = %li\n", numPathRouted);
    printf("Elapsed time    = %f seconds\n", TIMER_DIFF_SECONDS(startTime, stopTime));

    /*
     * Check solution and clean up
     */
    assert(numPathRouted <= numPathToRoute);
    bool status = maze_checkPaths(mazePtr, pathVectorListPtr, global_doPrint);
    assert(status == TRUE);
    puts("Verification passed.");
    maze_free(mazePtr);
    router_free(routerPtr);

    list_iter_reset(&it, pathVectorListPtr);
    while (list_iter_hasNext(&it, pathVectorListPtr)) {
        vector_t* pathVectorPtr = (vector_t*)list_iter_next(&it, pathVectorListPtr);
        vector_t *pointVectorPtr;
        while ((pointVectorPtr = (vector_t *)vector_popBack (pathVectorPtr)) != NULL) {
            PVECTOR_FREE(pointVectorPtr);
        }
        PVECTOR_FREE(pathVectorPtr);
    }
    list_free(pathVectorListPtr);
    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();
    TM_REPORT_ALL_STATS();
    return 0;
}


/* =============================================================================
 *
 * End of labyrinth.c
 *
 * =============================================================================
 */
