
#include <rte_ring.h>
#include <rte_errno.h>

#include "synergy_internal.h"
#include "debug.h"
#include "compiler.h"

/* Wait queue delay in us */
#ifndef CONGESTED_THRESH
#error "define CONGESTED_THRESH"
#endif

static struct rte_ring *wait_queue;
#ifdef ENABLE_TIMER_WORKER
static struct rte_ring *timer_wait_queue;
#endif
static uint64_t tsc_wqd_thresh;

/**
 * wraps_lt - a < b ?
 *
 * This comparison is safe against unsigned wrap around.
 *
 * "taken from shenango code base"
 */
static inline bool
wraps_lt ( uint32_t a, uint32_t b )
{
  return ( int32_t ) ( a - b ) < 0;
}

/* call this functions periodic times verifify if queue is congested by period
 * verified. If true, queue is congested.*/
bool
wait_queue_check_congested ( uint64_t __notused now )
{
#if CONGESTED_THRESH == 0
  return false;

#else
  static uint32_t last_producer = 0;
  static uint64_t next_check = 0;

  static bool congested = false;

  /* update congested status only if tsc_wqd period has passed and
   * current state is not congested. If current status is congested,
   * congested == true, force new verification to return as soon as possible
   * to congested == false. */
  if ( now < next_check && !congested )
    return false;

  next_check = now + tsc_wqd_thresh;

  uint32_t current_consumer = wait_queue->cons.tail;

  congested = wraps_lt ( current_consumer, last_producer );

  if ( !congested )
    {
      uint32_t current_producer = wait_queue->prod.tail;
      last_producer = current_producer;
    }

  return congested;
#endif
}

void
wait_queue_enqueue ( void *obj )
{
  if ( unlikely ( rte_ring_mp_enqueue ( wait_queue, obj ) ) )
    FATAL ( "Error enqueue on wait queue. Queue size is %u.\n"
            "Try increase len wait_queue.\n",
            rte_ring_count ( wait_queue ) );
}

#ifdef ENABLE_TIMER_WORKER
void
wait_queue_timer_enqueue ( void *obj )
{
  if ( unlikely ( rte_ring_sp_enqueue ( timer_wait_queue, obj ) ) )
    FATAL ( "Error enqueue on wait queue. Queue size is %u.\n"
            "Try increase len wait_queue.\n",
            rte_ring_count ( timer_wait_queue ) );
}
#endif

void *
wait_queue_dequeue ( void )
{
  void *obj = NULL;

#ifdef ENABLE_TIMER_WORKER
  if ( !rte_ring_mc_dequeue ( timer_wait_queue, &obj ) )
    return obj;
#endif

  rte_ring_mc_dequeue ( wait_queue, &obj );

  return obj;
}

unsigned
wait_queue_count ( void )
{
#ifdef ENABLE_TIMER_WORKER
  return rte_ring_count ( timer_wait_queue ) + rte_ring_count ( wait_queue );
#else
  return rte_ring_count ( wait_queue );
#endif
}

int
wait_queue_empty ( void )
{
  return rte_ring_empty ( wait_queue );
}

void
wait_queue_init ( void )
{
  // MC/MP queue
  wait_queue = rte_ring_create (
          "wait_queue", WAIT_QUEUE_SIZE, rte_socket_id (), 0 );

#ifdef ENABLE_TIMER_WORKER
  timer_wait_queue = rte_ring_create (
          "timer_wait_queue", WAIT_QUEUE_SIZE / 2, rte_socket_id (), 0 );
  if ( !wait_queue || !timer_wait_queue )
    FATAL ( "Error allocate wait queue: %s\n", rte_strerror ( rte_errno ) );
#else
  if ( !wait_queue )
    FATAL ( "Error allocate wait queue: %s\n", rte_strerror ( rte_errno ) );
#endif

  tsc_wqd_thresh = CONGESTED_THRESH * ( rte_get_timer_hz () / 1000000U );
}

void
wait_queue_clean ( void )
{
  rte_ring_free ( wait_queue );
#ifdef ENABLE_TIMER_WORKER
  rte_ring_free ( timer_wait_queue );
#endif
}
