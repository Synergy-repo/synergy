
#include <assert.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <rte_lcore.h>
#include <rte_cycles.h>
#include <rte_atomic.h>
#include <rte_spinlock.h>
#include <rte_timer.h>
#include <rte_random.h>
#include <rte_prefetch.h>

//#include "bitmask.h"
#include "interrupt.h"
#include "wait_queue.h"
#include "netio.h"
#include "debug.h"
#include "feedback.h"
#include "synergy_internal.h"
#include "compiler.h"
#include "globals.h"
#include "stats.h"
#include "timer.h"

/* quantum in us. */
#ifndef QUANTUM
#define QUANTUM 5
#endif

#ifndef QUANTUM_FACTOR
#define QUANTUM_FACTOR 1
#endif

/* no use congested diferrent factor if no explicit defined */
#ifndef CONGESTED_QUANTUM_FACTOR
#define CONGESTED_QUANTUM_FACTOR QUANTUM_FACTOR
#endif

struct wtimer
{
  _Atomic uint64_t deadline;
  _Atomic uint64_t last_tsc_quantum;
  _Atomic uint32_t state;
} __aligned ( CACHE_LINE_SIZE );

#define PREFETCH_OFFSET 3
static struct wtimer wtimers[MAX_WORKERS] = { 0 };

static uint64_t tsc_freq_us;

#ifdef ENABLE_QUANTUM_AUTOMATIC
static _Atomic unsigned quantum_factor = QUANTUM_FACTOR;
#else
static _Atomic unsigned quantum_factor = 1;
#endif
static _Atomic uint32_t *busy_workers;

#define TIMER_NOT_EXPIRED( now, deadline ) ( now < deadline )

static inline void
update_deadline ( struct wtimer *wt, uint64_t now )
{
  uint64_t previus_quanta = atomic_load ( &wt->last_tsc_quantum );

  atomic_store ( &( wt->deadline ), now + previus_quanta );
}

void
timer_set_quantum_factor ( unsigned congested )
{
  quantum_factor = ( congested ) ? CONGESTED_QUANTUM_FACTOR : QUANTUM_FACTOR;
}

void
timer_enable ( uint32_t wid, uint64_t quantum_tsc )
{
  uint64_t q = quantum_tsc * quantum_factor;
  uint64_t now = rte_get_tsc_cycles ();
  struct wtimer *wt = &wtimers[wid];

  atomic_store ( &wt->deadline, now + q );
  atomic_store ( &wt->last_tsc_quantum, q );

  wt->state = 1;
}

#ifdef CONCORD
extern __thread int concord_preempt_now;
#endif

/* Not ensure receive a interrupt after this function return */
void
timer_disable ( uint32_t wid )
{
  struct wtimer *wt = &wtimers[wid];
  wt->state = 0;

#ifdef CONCORD
  concord_preempt_now = 0;
#endif
}

#ifdef INT_STATS
static void
get_min_max_quanta ( uint64_t *mini, uint64_t *maxi )
{
  uint64_t min = atomic_load ( &wtimers[0].last_tsc_quantum );
  uint64_t max = atomic_load ( &wtimers[0].last_tsc_quantum );
  uint64_t t;

  for ( uint32_t i = 1; i < tot_workers; i++ )
    {
      t = atomic_load ( &wtimers[i].last_tsc_quantum );
      min = ( t < min ) ? t : min;
      max = ( t > max ) ? t : max;
    }

  *mini = min / ( double ) ( tsc_freq_us / 1000.0 );
  *maxi = max / ( double ) ( tsc_freq_us / 1000.0 );
}
#endif

static uint64_t
get_tsc_avg_quantum ( void )
{
  uint64_t n = atomic_load ( &wtimers[0].last_tsc_quantum );

  for ( uint32_t i = 1; i < tot_workers; i++ )
    n += atomic_load ( &wtimers[i].last_tsc_quantum );

  return n / tot_workers;
}

uint64_t
timer_get_avg_us_quantum ( void )
{
#ifdef QUANTUM_AUTOMATIC
  return get_tsc_avg_quantum () / ( double ) ( tsc_freq_us / 1000.0 );
#else
  return 0.0;
#endif
}

void
timer_do_interrupts ( uint64_t now )
{

#ifdef ENABLE_WORKSTEALING_BW
  _Atomic uint32_t *bwp = busy_workers;
#endif

  unsigned w;
  struct wtimer *wt, *wt_prefetch;
  wt_prefetch = wtimers;
  for ( w = 0; w < tot_workers; w++ )
    {
      /* pointer arithmethic to avoid use of &wtimers[w + size] that can access
       * elements behind of array*/
      rte_prefetch0 (
              ( volatile void * ) ( wt_prefetch + w + PREFETCH_OFFSET ) );

      wt = &wtimers[w];

      if ( wt->state == DISABLED )
        continue;

      if ( TIMER_NOT_EXPIRED ( now, wt->deadline ) )
        {
#ifdef ENABLE_WORKSTEALING_BW
          *bwp++ = w;
#endif
          continue;
        }

#ifdef INT_STATS
      stats_interrupt_targed++;
#endif

#ifdef ENABLE_CONDITIONAL_INTERRUPT
      if ( !netio_worker_has_new_jobs ( w ) )
        {
          /* Renew worker timer only updating deadline whitout touch in state
           * because this can may be manipulated by worker (i.e disabling
             timer); */
          update_deadline ( wt, now );
#ifdef ENABLE_WORKSTEALING_BW
          *bwp++ = w;
#endif
          continue;
        }
#endif

      /* Critical section: if exchange return false than worker set this as
       * DISABLED or PAUSED (i.e call timer_disable()) */
      uint32_t expected = ACTIVE;
      if ( atomic_compare_exchange_strong (
                   &( wt->state ), &expected, DISABLED ) )
        {
#ifdef INT_STATS
          stats_interrupt_fired++;
#endif
          /* Add worker to interrupt after of checked all workers */
#ifdef KMOD_IPI
          interrupt_add_worker ( w );
#else
          interrupt_send ( w );
#endif
        }
    }

#ifdef ENABLE_WORKSTEALING_BW
  *bwp = -1;
#endif

#ifdef KMOD_IPI
  /* Interrupt all worker with expired timer */
  if ( interrupt_has_worker_to_interrupt () )
    {
      interrupt_many_workers ();
      interrupt_clear_workers ();
    }
#endif
}

static void
info ( void )
{
  INFO ( "Starting timer core on core %u.\n", rte_lcore_id () );
  INFO ( "TSC frequency: %lu ticks per us.\n", tsc_freq_us );

#ifdef ENABLE_WAIT_QUEUE
  INFO ( "%s\n", "WAIT QUEUE: ENABLE" );
#else
  INFO ( "%s\n", "WAIT QUEUE: DISABLE" );
#endif

#ifdef ENABLE_INTERRUPT
  INFO ( "%s\n", "INTERRUPT: ENABLE" );

#if defined CONCORD
  INFO ( "%s\n", "WORKER NOTIFY: CI" );
#elif defined SIGNAL
  INFO ( "%s\n", "WORKER NOTIFY: SIGNAL" );
#elif defined UINTR
  INFO ( "%s\n", "WORKER NOTIFY: UINTR" );
#elif defined KMOD_IPI
  INFO ( "%s\n", "WORKER NOTIFY: KMOD_IPI" );
#endif

#ifdef QUANTUM_AUTOMATIC
  INFO ( "%s", "QUANTUM AUTOMATIC: ENABLE.\n" );
  INFO ( "QUANTUM STARTING: %u us.\n", QUANTUM );
#else
  INFO ( "%s", "QUANTUM AUTOMATIC: DISABLE.\n" );
  INFO ( "QUANTUM: %u us.\n", QUANTUM );
#ifdef ENABLE_CONDITIONAL_INTERRUPT
  INFO ( "%s\n", "CONDITIONAL INTERUPT: ENABLE" );
#else
  INFO ( "%s\n", "CONDITIONAL INTERUPT: DISABLE" );
#endif

#endif

#else
  INFO ( "%s\n", "INTERRUPT: DISABLE" );
#endif  // ENABLE_INTERRUPT

  INFO ( "QUANTUM FACTOR: %u\n", QUANTUM_FACTOR );
  INFO ( "CONGESTED_THRESH %u\n", CONGESTED_THRESH );
  INFO ( "CONGESTED QUANTUM FACTOR: %u\n", CONGESTED_QUANTUM_FACTOR );
}

void
timer_init ( void )
{
  tsc_freq_us = rte_get_timer_hz () / 1000000U;
  busy_workers = netio_busy_workers ();

  info ();
}
