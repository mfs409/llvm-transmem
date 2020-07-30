/* Copyright (c) IBM Corp. 2014. */
#ifndef HLE_INTEL_H
#define HLE_INTEL_H 1

extern void tm_startup_hle();
extern void tm_shutdown_hle();

extern void tm_thread_enter_hle();
extern void tm_thread_exit_hle();

extern void tbegin_hle(int region_id);
extern void tend_hle();
extern void tabort_hle();

#endif
