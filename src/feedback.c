
#include <rte_cycles.h>

#include "feedback.h"
#include "globals.h"
#include "timer.h"
#include "stats.h"

static __thread uint64_t start_short_request_tsc, avg_shorts_service_time_tsc;

static uint64_t
calc_moving_avg ( uint64_t current_avg, uint64_t new_value )
{
  /* standard moving avg */
  // return ( ( tot_samples - 1 ) * current_avg + new_value ) / tot_samples;

  /* Welford's method  moving avg */
  // return current_avg + ( ( new_value - current_avg ) / tot_samples );

  /* Exponentially Weighted Moving Average (EWMA) */
  //#define ALPHA 0.5
  // return ALPHA * new_value + ( 1 - ALPHA ) * current_avg;
  return ( current_avg + new_value ) / 2;
}

void
synergy_feedback_start_long ( void )
{
#ifdef ENABLE_FEEDBACK
  CURRENT_SET_LONG ();
  timer_enable ( worker_id, avg_shorts_service_time_tsc );
#endif
}

void
synergy_feedback_finished_long ( void )
{
#ifdef ENABLE_FEEDBACK
  CURRENT_SET_FINISHED ();
  timer_disable ( worker_id );
#endif
}

static void
synergy_feedback_start_short ( void )
{
#ifdef ENABLE_FEEDBACK
  CURRENT_SET_SHORT ();
  start_short_request_tsc = rte_get_tsc_cycles ();
#endif
}

void
synergy_feedback_start_request ( void )
{
  CURRENT_SET_SHORT ();
  /* assume that new requests are short requests */
  synergy_feedback_start_short ();
}

static void
synergy_feedback_finished_short ( void )
{
#ifdef ENABLE_FEEDBACK
  CURRENT_SET_FINISHED ();
#ifdef QUANTUM_AUTOMATIC
  uint64_t st = rte_get_tsc_cycles () - start_short_request_tsc;

  avg_shorts_service_time_tsc =
          calc_moving_avg ( avg_shorts_service_time_tsc, st );
#endif
#endif
}

void
synergy_feedback_finished_request ( void )
{
#ifdef CPU_STATS
  uint64_t now = rte_get_tsc_cycles ();
  total_app_time += now - app_start_time;
  uint64_t tot_run_tmp = now - start_runtime;
  cpu_utilization[worker_id] += total_app_time / ( double ) tot_run_tmp;
  cpu_utilization[worker_id] /= 2.0;
#endif

#ifdef ENABLE_FEEDBACK
  CURRENT_SET_FINISHED ();
  if ( CURRENT_IS_SHORT () )
    {
      synergy_feedback_finished_short ();
    }

#else

  /* if no feedback, need disable the timer after request finished.
   */
  CURRENT_SET_FINISHED ();
  timer_disable ( worker_id );
#endif
}

void
feedback_init_per_worker ( void )
{
  avg_shorts_service_time_tsc =
          QUANTUM * ( rte_get_timer_hz () / ( 1000000U ) );
}

uint64_t
feedback_get_shorts_tsc_avg ( void )
{
  return avg_shorts_service_time_tsc;
}
