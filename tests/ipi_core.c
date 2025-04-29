
#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <x86intrin.h>

#include <signal.h>
#include <sys/io.h>

#include "util.h"
#include "ipi.h"

// ensure not using hyperthreads of same core
#define SENDER_CORE 2
#define WORKER_CORE 4

#define RUNS 500000U

static volatile int worker_ready, worker_stop;

static uint32_t tsc_sender[RUNS], timer_frequency[RUNS];
static uint32_t tsc_worker[RUNS], tsc_sender_worker[RUNS];
static uint32_t tsc_worker_worker[RUNS];

static uint64_t volatile sender_start_tsc, worker_start_tsc,
        worker_start_in_handler;

static volatile bool wait_worker = 1, handler_checkout;

static volatile uint32_t i_handler = 0;

void
handler ( void )
{
  static uint64_t last_now = 0;
  uint64_t now = _rdtsc ();

  tsc_sender_worker[i_handler] = now - sender_start_tsc;
  tsc_worker[i_handler] = now - worker_start_tsc;

  timer_frequency[i_handler] = now - last_now;
  last_now = now;

  i_handler++;
  worker_start_in_handler = _rdtsc ();
  handler_checkout = 1;
}

void *
worker ( void *arg )
{
  pin_to_cpu ( WORKER_CORE );
  printf ( "Started worker thread on core %u\n", sched_getcpu () );

  ipi_init_worker ();
  uint32_t i = 0;

  worker_ready = 1;
  while ( !worker_stop )
    {
      worker_start_tsc = _rdtsc ();
      if ( handler_checkout )
        {
          tsc_worker_worker[i++] = _rdtsc () - worker_start_in_handler;
          handler_checkout = 0;
          wait_worker = 0;
        }
    }

  return NULL;
}

static void
sender ( void )
{
  pin_to_cpu ( SENDER_CORE );
  printf ( "Started sender thread on core %u\n", sched_getcpu () );

  for ( unsigned int i = 0; i < RUNS; i++ )
    {
      sender_start_tsc = _rdtsc ();

      ipi_send_interrupt ( WORKER_CORE );

      tsc_sender[i] = _rdtsc () - sender_start_tsc;

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
  puts ( "here" );

  printf ( "Samples: %u\n"
           "Cycles by us: %u\n"
           "TSC overhead: %lu\n",
           RUNS,
           cycles_by_us,
           rdtsc_overhead );

  if ( ipi_init ( handler ) < 0 )
    {
      fprintf ( stderr, "Error tra_init\n" );
      return 1;
    }

  pthread_t tid;
  pthread_create ( &tid, NULL, worker, NULL );

  while ( !worker_ready )
    ;

  sender ();
  pthread_join ( tid, NULL );
  ipi_free ();

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

  printf ( "\nIPIs handled: %d\n", i_handler );
  return 0;
}
