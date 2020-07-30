// TODO: consider moving this, and countingTM, to the tests folder?

// Statistics are not a fundamental feature of TM libraries.  However, it
// may be useful to have a mechanism for counting library statistics, especially
// for some unit tests.
#pragma once

// Everything in the loibrary statistics package has C linkage
extern "C" {

// Application code can call these functions, and libraries may implement them
#define TM_STATS_GET tm_stats_get
#define TM_STATS_RESET tm_stats_reset

// The names of the statistics we might track
enum TM_STATS {
  TM_STATS_LOAD_U1 = 0,             // 8-bit int load
  TM_STATS_LOAD_U2 = 1,             // 16-bit int load
  TM_STATS_LOAD_U4 = 2,             // 32-bit int load
  TM_STATS_LOAD_U8 = 3,             // 64-bit int load
  TM_STATS_LOAD_F = 4,              // 32-bit float load
  TM_STATS_LOAD_D = 5,              // 64-bit float load
  TM_STATS_LOAD_LD = 6,             // 128-bit (80-bit on Intel) float load
  TM_STATS_LOAD_P = 7,              // pointer load (assume 64-bit)
  TM_STATS_STORE_U1 = 8,            // 8-bit int store
  TM_STATS_STORE_U2 = 9,            // 16-bit int store
  TM_STATS_STORE_U4 = 10,           // 32-bit int store
  TM_STATS_STORE_U8 = 11,           // 64-bit int store
  TM_STATS_STORE_F = 12,            // 32-bit float store
  TM_STATS_STORE_D = 13,            // 64-bit float store
  TM_STATS_STORE_LD = 14,           // 128-bit (80-bit on Intel) float store
  TM_STATS_STORE_P = 15,            // pointer store (assume 64-bit)
  TM_STATS_BEGIN_OUTER = 16,        // start non-nested transaction
  TM_STATS_BEGIN_INNER = 17,        // start nested transaction
  TM_STATS_END_OUTER = 18,          // commit non-nested transaction
  TM_STATS_END_INNER = 19,          // commit nested transaction
  TM_STATS_MALLOCS = 20,            // malloc() from within a transaction
  TM_STATS_FREES = 21,              // free() from within a transaction
  TM_STATS_TRANSLATE_FOUND = 22,    // translate function pointer found
  TM_STATS_TRANSLATE_NOTFOUND = 23, // translate function pointer not found
  TM_STATS_REGCLONE = 24,           // register a function clone
  TM_STATS_UNSAFE = 25,             // an unsafe operation happened
  TM_STATS_MEMCPY = 26,             // memcpy() from within a transaction
  TM_STATS_MEMSET = 27,             // memset() from within a transaction
  TM_STATS_MEMMOVE = 28,            // memmove() from within a transaction
  TM_STATS_READONLY = 29,           // read-only transaction
  TM_STATS_ALIGNED_ALLOC = 30,      // aligned_alloc() from within a transaction
  TM_STATS_COUNT = 31,
};

// TODO change this so it doesn't have to be static
static const char *TM_STAT_NAMES[] = {
    "TM_STATS_LOAD_U1",         "TM_STATS_LOAD_U2",
    "TM_STATS_LOAD_U4",         "TM_STATS_LOAD_U8",
    "TM_STATS_LOAD_F",          "TM_STATS_LOAD_D",
    "TM_STATS_LOAD_LD",         "TM_STATS_LOAD_P",
    "TM_STATS_STORE_U1",        "TM_STATS_STORE_U2",
    "TM_STATS_STORE_U4",        "TM_STATS_STORE_U8",
    "TM_STATS_STORE_F",         "TM_STATS_STORE_D",
    "TM_STATS_STORE_LD",        "TM_STATS_STORE_P",
    "TM_STATS_BEGIN_OUTER",     "TM_STATS_BEGIN_INNER",
    "TM_STATS_END_OUTER",       "TM_STATS_END_INNER",
    "TM_STATS_MALLOCS",         "TM_STATS_FREES",
    "TM_STATS_TRANSLATE_FOUND", "TM_STATS_TRANSLATE_NOTFOUND",
    "TM_STATS_REGCLONE",        "TM_STATS_UNSAFE",
    "TM_STATS_MEMCPYS",         "TM_STATS_MEMSETS",
    "TM_STATS_MEMMOVES",        "TM_STATS_PLACEHOLDER",
    "TM_STATS_READONLY",        "TM_STATS_ALIGNED_ALLOC", 
};

// We provide a declaration for the statistics getter function, so that programs
// that require statistics from their library can compile without errors
int TM_STATS_GET(TM_STATS e);

// We provide a declaration for the statistics reset function, so that programs
// that require statistics from their library can compile without errors
void TM_STATS_RESET();
}
