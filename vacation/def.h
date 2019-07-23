#pragma once

#include "../include/tm.h"

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_CALLABLE                   TX_SAFE
#define TM_THREAD_ENTER()             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */
#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */
#define GOTO_SIM()                    /* nothing */
#define GOTO_REAL()                   /* nothing */
#define P_MEMORY_STARTUP(numThread)   /* nothing */ 
#define P_MEMORY_SHUTDOWN()           /* nothing */
#define SIM_GET_NUM_CPU(var)          /* nothing */
#define P_MALLOC(size)                malloc(size)
#define P_FREE(ptr)                   free(ptr)
#define TM_RESTART()                  return false

#define TM_SHARED_READ(var)           var
#define TM_SHARED_READ_P(var)         var
#define TM_SHARED_READ_F(var)         var

#define TM_SHARED_WRITE(var, val)     var = val
#define TM_SHARED_WRITE_P(var, val)   var = val
#define TM_SHARED_WRITE_F(var, val)   var = val

#define TM_LOCAL_WRITE(var, val) var = val
#define TM_LOCAL_WRITE_P(var, val)    var = val

extern "C" {
void TM_REPORT_ALL_STATS();
extern TX_PURE int strncmp(__const char *__s1, __const char *__s2,
                           size_t __n) __THROW __attribute_pure__
    __nonnull((1, 2));
}
