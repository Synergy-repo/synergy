
#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdatomic.h>

#include <rte_mbuf.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>

#include "globals.h"
#include "dpdk.h"
#include "debug.h"
#include "netio.h"
#include "interrupt.h"
#include "timer.h"
#include "synergy_internal.h"
#include "feedback.h"
#include "stats.h"
#include "flow.h"
#include "thread.h"
#include "wait_queue.h"
#include "synergy.h"

#define PORT_ID 0

/* APP function callback to handle requests */
static app_func app_cb;
static void *app_arg;

static __thread struct thread th_main, *th_worker;

/* this is used to lazy free green threads */
static __thread unsigned tot_threads_to_free;
static __thread struct thread *
        threads_to_free[THREAD_LAZY_FREE_SIZE] __aligned ( CACHE_LINE_SIZE );

static _Atomic unsigned workers_active;

#ifndef ENABLE_FEEDBACK
static uint64_t tsc_quantum;
#endif

enum from
{
  PREEMPTED = 0,
  SWAP,
  TH_EXIT
};

static __thread enum from from;

void __nofp
yield_to_main ( void )
{
  assert ( CURRENT_IS_LONG () );
#ifdef CPU_STATS
  total_app_time += rte_get_tsc_cycles () - app_start_time;
#endif

  /* save worker context and jump to th_main. The th_main will enqueue
   * this worker context and create new worker context. Next time this
   * context run, this starting after this call and return to original
   * app code */
  from = PREEMPTED;
  CURRENT_SET_MAIN ();
  thread_switch_extended ( th_worker, &th_main );

#ifdef ENABLE_FEEDBACK
  /* Before restore worker context, than is handling a long request, mark
   * current as long and enable timer.*/
  synergy_feedback_start_long ();
#else
  CURRENT_SET_LONG ();
  timer_enable ( worker_id, tsc_quantum );
#endif
#ifdef CPU_STATS
  app_start_time = rte_get_tsc_cycles ();
#endif
}

/* signal or kmod_ipi interrupt
 * TODO: GCC 14 has new attribute function 'no_callee_saved_registers'
 * that is useful here when using kmod_ipi */
void __nofp
interrupt_handler ( int __notused x )
{
  STAT ( interrupts, 1 );

#ifdef THREAD_DEBUG
  th_worker->preempt_count++;
#endif

  /* Current user thread (that is a long request) cannot be rescheduling to
   * other core. See comment in globals.c */
  if ( CURRENT_IS_RESCHEDULING_DISABLED () )
    {
      STAT ( invalid_interrupts, 1 );

      /* feedback STOP FINISHED not ensure that timer is disabled and one
       * interrupt can be in fly.then the current context can be a main, short
       * or finished context less a long context. */
      if ( !CURRENT_IS_LONG () )
        return;

      /* current context is a long request that cannot be rescheduling. Set this
       * to yield when finished critical section. */
      CURRENT_SET_YIELD ();
      STAT ( yield, 1 );

      return;
    }

  yield_to_main ();
}

/* user thread entry point */
static void
thread_start ( void )
{
  CURRENT_SET_MAIN ();
  app_cb ( app_arg );
}

/* This is called when user thread return */
static void
thread_exit ( void )
{
  CURRENT_SET_MAIN ();
  from = TH_EXIT;
  thread_set ( &th_main );

  __unreachable ();
}

static _Atomic int wait_queue_congested;

static void
timer_sched ( void )
{
  timer_init ();

#ifdef ENABLE_TIMER_WORKER
  worker_id = tot_workers;
  netio_init_per_worker ( worker_id );
  feedback_init_per_worker ();
#endif

  while ( likely ( !quit ) )
    {
      uint64_t now = rte_get_tsc_cycles ();

#ifdef ENABLE_WAIT_QUEUE
      int congested = !!wait_queue_check_congested ( now );
      if ( congested != wait_queue_congested )
        {
          timer_set_quantum_factor ( congested );
          atomic_store ( &wait_queue_congested, congested );
        }
#endif

#ifdef ENABLE_INTERRUPT
      timer_do_interrupts ( now );
#endif
    }
}

/* swap to 'th' and handle if current 'th_worker' will be free now or its go be
 * lazy freed */
__noreturn static void
swap_direct ( struct thread *th )
{
  /* lazy green thread free */
  threads_to_free[tot_threads_to_free++] = th_worker;

  /* set new thread */
  th_worker = th;

  /* when a green thread is swaped to other green thread taken from wait queue,
   * the current green thread is dead and is put on threads_to_free array. This
   * array is freed when a long request is interrupt or when the array is full.
   * Then, is expected that tot_threads_to_free be minor that
   * THREAD_LAZY_FREE_SIZE in many occurrences.
   *
   * The other case can ocorr when a long request finished and the
   * current green threads is used to process a new request and the dead thread
   * in threads_to_free is lazy freed because interrupt not occurs. Then, this
   * case occurs with minor frequency that interrupts.
   *
   * Lazy free dead threads can improve performance since that this happen in
   * bulk
   */

  if ( likely ( tot_threads_to_free < THREAD_LAZY_FREE_SIZE ) )
    {
      /* direct jump to new app green thread */
      thread_set ( th_worker );
    }
  else
    {
      /* main thread should free all green threads and schedule 'th_worker'*/
      CURRENT_SET_MAIN ();
      from = SWAP;
      thread_set ( &th_main );
    }

  __unreachable ();
}

int
get_request ( void **data, struct sock *sock )
{
  struct rte_mbuf *pkt;
  struct thread *th;
  uint16_t size;

  while ( likely ( !quit ) )
    {
#ifdef ENABLE_WAIT_QUEUE
      if ( unlikely ( wait_queue_congested == 1 ) )
        {
          STAT ( avoid_starvation, 1 );
          goto check_wait_queue;
        }
#endif

      pkt = netio_recv ();
      if ( pkt )
        goto done;

#ifdef ENABLE_WAIT_QUEUE
    check_wait_queue:
      th = wait_queue_dequeue ();
      if ( th )
        swap_direct ( th );
#endif

#ifdef ENABLE_WORKSTEALING
      pkt = netio_workstealing ();
      if ( pkt )
        goto done;
#endif

      continue;

    done:

#ifndef ENABLE_WAIT_QUEUE
      /* ugly hack to identify if is a packet or green thread preempted. this is
       * only necessary when not using wait queue.
       * sizeof(struct rte_mbuf) == 128 and offsetof(struct thread, stack) == 64
       * . Then this is safe on my setup. Also assume that stack overflow not
       * occurs */
      if ( *( ( struct thread * ) pkt )->stack == CANARY )
        swap_direct ( ( struct thread * ) pkt );
#endif

      // if ( likely ( !netio_parse_pkt ( pkt, data, &size ) ) )
      netio_parse_pkt ( pkt, data, &size );
      sock->pkt = pkt;
#ifndef ENABLE_FEEDBACK
      /* if no feedback, need enable the timer before schedule any
       * request. */
      CURRENT_SET_LONG ();
      timer_enable ( worker_id, tsc_quantum );
#endif
      /* start feedback when a new request arrive. this is, currentily,
       * closed on netio_send. */
      synergy_feedback_start_request ();
#ifdef CPU_STATS
      app_start_time = rte_get_tsc_cycles ();
#endif
      return size;
    }

  return -1;
}

static void
free_dead_threads ( void )
{
  thread_free_bulk ( threads_to_free, tot_threads_to_free );
  tot_threads_to_free = 0;
}

static void
worker_main_sched ( void )
{
#ifdef CPU_STATS
  start_runtime = rte_get_tsc_cycles ();
#endif

sched_new:
  th_worker = thread_create ( thread_start, thread_exit );
  thread_switch ( &th_main, th_worker );

  switch ( from )
    {
        /* enqueue long request */
      case PREEMPTED:
#ifdef ENABLE_WAIT_QUEUE
        wait_queue_enqueue ( th_worker );
#else
        netio_worker_enqueue ( th_worker );
#endif
        if ( tot_threads_to_free )
          free_dead_threads ();
        goto sched_new;
        break;
        /* free old green threads and swap to new green threads */
      case SWAP:
        free_dead_threads ();
        thread_set ( th_worker );
        __unreachable ();
        break;
      case TH_EXIT:
        thread_free ( th_worker );
        break;
    }
}

static int
worker ( void *arg )
{
  worker_id = ( uint16_t ) ( uintptr_t ) arg;

  if ( rte_eth_dev_socket_id ( PORT_ID ) >= 0 &&
       rte_eth_dev_socket_id ( PORT_ID ) != ( int ) rte_socket_id () )
    {
      WARNING ( "port %u is on remote NUMA node to "
                "polling thread.\n\tPerformance will "
                "not be optimal.\n",
                PORT_ID );
    }

  netio_init_per_worker ( worker_id );
  interrupt_register_worker ( worker_id );
  feedback_init_per_worker ();

  INFO ( "Starting worker %u on core %u.\n", worker_id, rte_lcore_id () );

  workers_active++;

  while ( workers_active != tot_workers )
    ;

  allocator_on = true;
  worker_main_sched ();
  allocator_on = false;

#ifdef UINTR
  interrupt_free_worker ( worker_id );
#endif

  show_stats_per_worker ();

  workers_active--;

  return 0;
}

static void
workers_init ( void )
{
  uint16_t wid = 0;

  unsigned int core_id;
  RTE_LCORE_FOREACH_WORKER ( core_id )
  {
    if ( rte_eal_remote_launch (
                 worker, ( void * ) ( uintptr_t ) wid++, core_id ) )
      FATAL ( "Error start worker on core id %d: %s",
              core_id,
              rte_strerror ( rte_errno ) );
  }
}

static void
finish ( void )
{
  while ( workers_active > 0 )
    asm volatile( "pause" );

  show_stats_timer ();
  show_stats ( PORT_ID );

  /* clean ups */
  netio_clean ();
  thread_clean ();
  wait_queue_clean ();

  rte_eal_cleanup ();
}

int
// main ( int argc, char **argv )
synergy_init ( int argc, char **argv, app_func cb, void *cb_arg )
{
  app_cb = cb;
  app_arg = cb_arg;

  if ( rte_eal_init ( argc, argv ) < 0 )
    FATAL ( "%s\n", "Cannot init EAL\n" );

  if ( rte_lcore_count () < 2 )
    FATAL ( "%s", "Minimmum of cores should be 2" );

  INFO ( "Cores available %d. Main core %d.\n",
         rte_lcore_count (),
         rte_get_main_lcore () );

  tot_workers = rte_lcore_count () - 1;
  if ( tot_workers > MAX_WORKERS )
    FATAL ( "%s", "Increase MAX_WORKERS limit" );

  /* inits */
  wait_queue_init ();

  uint16_t tot_txq = tot_workers;
  uint16_t tot_rxq = tot_workers;
#ifdef ENABLE_TIMER_WORKER
  tot_txq += 1;
#endif

#ifdef ENABLE_CFCFS
  tot_rxq = 1;
#endif

  netio_init ( PORT_ID, tot_rxq, tot_txq );

  interrupt_init ( interrupt_handler );
  thread_init ( NULL, NULL );
  // flow_init ( PORT_ID, tot_rxq );

  workers_init ();

#ifndef ENABLE_FEEDBACK
  tsc_quantum = QUANTUM * ( rte_get_timer_hz () / 1000000 );
#endif

  while ( workers_active != tot_workers )
    ;

  allocator_on = true;
  timer_sched ();
  allocator_on = false;

  finish ();

  return 0;
}
