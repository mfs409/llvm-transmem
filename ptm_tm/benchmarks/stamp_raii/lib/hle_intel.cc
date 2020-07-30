/* Copyright (c) IBM Corp. 2014. */
/* Code that uses Intel Hardware Lock Elision: derived from htm_ibm.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <immintrin.h>
#include "hle_intel.h"
#include "thread.h"

#define NUM_ATOMIC_REGIONS 20

int global_lock;
static THREAD_KEY_T global_tls_key;
typedef struct event_counters_struct {
	unsigned long long event_tx_enter;
	unsigned long long event_tx;
} event_counters_t;
typedef struct tls_struct {
	event_counters_t event_counters[NUM_ATOMIC_REGIONS];
} tls_t;
static event_counters_t global_event_counters;
static event_counters_t global_ctr_per_region[NUM_ATOMIC_REGIONS];
static int global_evt_ctrs_lock = 0;

void tm_startup_hle() {
	__atomic_store_n(&global_lock,0,__ATOMIC_RELEASE);
	THREAD_KEY_INIT(global_tls_key);
	
	memset(&global_event_counters,0,sizeof(global_event_counters));
	memset(&global_ctr_per_region,0,sizeof(global_ctr_per_region));
}

static void print_stats_hle(event_counters_t *stats) {
	fprintf(stderr, "#HTM_STATS %15llu           tx_enter\n", stats->event_tx_enter);
	fprintf(stderr, "#HTM_STATS %15llu %6.2f %%  tx\n", stats->event_tx, 100 * stats->event_tx / (double)stats->event_tx_enter);
}

void tm_shutdown_hle() {
	int region;
	for(region=0; region<NUM_ATOMIC_REGIONS; region++) {
		global_event_counters.event_tx_enter+=global_ctr_per_region[region].event_tx_enter;
		global_event_counters.event_tx+=global_ctr_per_region[region].event_tx;
	}
	print_stats_hle(&global_event_counters);
	for(region=0; region<NUM_ATOMIC_REGIONS; region++) {
		if(global_ctr_per_region[region].event_tx_enter) {
			fprintf(stderr, "--- region %d ------------------------------\n", region);
			print_stats_hle(&global_ctr_per_region[region]);
		}
	}
}

void tm_thread_enter_hle() {
	tls_t *tls;
	if(!(tls=malloc(sizeof(tls_t)))) {
		perror("malloc tls");
		exit(1);
	}
	memset(tls,0,sizeof(tls_t));
	THREAD_KEY_SET(global_tls_key,tls);
}

void tm_thread_exit_hle() {
	tls_t *tls;
	tls=THREAD_KEY_GET(global_tls_key);
	int region;
	/* Lock Global counters */
	while(__atomic_exchange_n(&global_evt_ctrs_lock,1,__ATOMIC_ACQUIRE|__ATOMIC_HLE_ACQUIRE))
		_mm_pause();
	for(region=0; region<NUM_ATOMIC_REGIONS; region++) {
		global_ctr_per_region[region].event_tx_enter += tls->event_counters[region].event_tx_enter;
		global_ctr_per_region[region].event_tx += tls->event_counters[region].event_tx;
	}
	/* unlock global counters */
	__atomic_store_n(&global_evt_ctrs_lock,0,__ATOMIC_RELEASE|__ATOMIC_HLE_RELEASE);
	free(tls);
}

void tbegin_hle(int region_id) {
	tls_t *tls;
	tls=THREAD_KEY_GET(global_tls_key);
	tls->event_counters[region_id].event_tx_enter++;
	while(__atomic_exchange_n(&global_lock,1,__ATOMIC_ACQUIRE|__ATOMIC_HLE_ACQUIRE))
		_mm_pause();
	if(!_xtest()) {
		tls->event_counters[region_id].event_tx++;
	}
	return;
}

void tend_hle() {
	__atomic_store_n(&global_lock,0,__ATOMIC_RELEASE|__ATOMIC_HLE_RELEASE);
}

void tabort_hle() {
  asm volatile(".byte 0xc6; .byte 0xf8; .byte 0xff" :: );
  /*__builtin_ia32_xabort(0xff);*/
}
