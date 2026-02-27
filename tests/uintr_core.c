
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#include <unistd.h>
#include <signal.h>
#include <x86intrin.h>

#include <sys/syscall.h>

#include "util.h"

#include "../kmod/uintr.h"

#define SENDER_CORE 2
#define WORKER_CORE 4

#define RUNS 500000U

static volatile int worker_ready, worker_stop;

static uint32_t tsc_sender[RUNS], timer_frequency[RUNS];
static uint32_t tsc_worker[RUNS], tsc_sender_worker[RUNS];
static uint32_t tsc_worker_worker[RUNS];

static uint64_t volatile sender_start_tsc, worker_start_tsc,
        worker_start_in_handler;

static volatile int wait_worker = 1, handler_checkout;

static volatile uint32_t i_handler = 0;

static int worker_idx;

static void
handler ( void )
{
  static uint64_t last_now = 0;
  uint64_t now = __rdtsc ();

  tsc_sender_worker[i_handler] = now - sender_start_tsc;
  tsc_worker[i_handler] = now - worker_start_tsc;

  timer_frequency[i_handler] = now - last_now;
  last_now = now;

  i_handler++;
  worker_start_in_handler = _rdtsc ();
  handler_checkout = 1;
}

static void *
worker ( void *arg )
{
  pin_to_cpu ( WORKER_CORE );
  printf ( "Started worker thread on core %u\n", sched_getcpu () );

  int worker_idx = uintr_init_worker ();

  uint32_t i = 0;

  worker_ready = 1;
  while ( !worker_stop )
    {
      worker_start_tsc = __rdtsc ();
      if ( handler_checkout )
        {
          tsc_worker_worker[i++] = _rdtsc () - worker_start_in_handler;
          handler_checkout = 0;
          wait_worker = 0;
        }
    }

  uintr_free_worker ( worker_idx );

  return NULL;
}

static void
sender ( void )
{
  pin_to_cpu ( SENDER_CORE );
  printf ( "Started sender thread on core %u\n", sched_getcpu () );

  /* should init after pin core */
  if ( uintr_init_sender () )
    {
      fprintf ( stderr, "%s\n", "Error to init sender" );
      exit ( 1 );
    }

  for ( unsigned int i = 0; i < RUNS; i++ )
    {
      sender_start_tsc = __rdtsc ();

      uintr_senduipi ( worker_idx );

      tsc_sender[i] = __rdtsc () - sender_start_tsc;

      while ( wait_worker )
        ;

      wait_worker = 1;
    }

  worker_stop = 1;
}

int
main ( int argc, char **argv )
{
  init_util ();

  printf ( "Samples: %u\n"
           "Cycles by us: %u\n"
           "TSC overhead: %lu\n",
           RUNS,
           cycles_by_us,
           rdtsc_overhead );

  if ( uintr_init ( handler ) )
    {
      fprintf ( stderr, "%s\n", "Error to open kmod_uintr" );
      return 1;
    }

  pthread_t tid;
  pthread_create ( &tid, NULL, worker, NULL );

  while ( !worker_ready )
    ;

  sender ();
  pthread_join ( tid, NULL );

  uintr_free ();

  print ( tsc_sender, RUNS, "Sender (Time to call 'send')", 0 );
  print ( tsc_sender_worker,
          RUNS,
          "Sender/Worker (Time between call 'send' and the interrupt arrive on "
          "receiver)",
          0 );
  print ( tsc_worker,
          RUNS,
          "Worker (Time to receiver of interrupt change from main flow to "
          "interrupt flow)",
          0 );
  print ( tsc_worker_worker,
          RUNS,
          "Worker/Worker (Time to receiver of interrupt change go back from "
          "interrupt flow to main flow)",
          0 );
  print ( timer_frequency, RUNS, "Timer frequency", 1 );

  printf ( "\nSignals handled: %d\n", i_handler );

  return 0;
}
