
#ifndef STATS_H
#define STATS_H

#include <stdint.h>
#include "synergy_internal.h"

/* per worker stats */
extern __thread uint64_t rx_count, rx_dropped, interrupts, invalid_interrupts,
        stealing, avoid_starvation, worker_memory, worker_lock, worker_free,
        yield;

extern uint64_t timer_starvation_avoid, timer_wait_queue_processed,
        timer_processed;

//#define INT_STATS
//#define CPU_STATS

#ifdef INT_STATS
extern uint64_t stats_interrupt_targed;
extern uint64_t stats_interrupt_fired;
#endif

#ifdef CPU_STATS
extern double cpu_utilization[MAX_WORKERS];
extern __thread uint64_t start_runtime;
extern __thread uint64_t total_app_time;
extern __thread uint64_t app_start_time;
#endif

#define STAT( x, v ) ( ( x ) += ( v ) )

void
show_stats ( int portid );

void
show_stats_per_worker ( void );

void
show_stats_timer ( void );

#endif
