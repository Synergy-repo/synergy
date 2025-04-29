#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <x86intrin.h>

#include "util.h"

#include "libdune/dune.h"
#include "libdune/cpu-x86.h"
#include "libdune/local.h"

#define SENDER_CORE 5
#define WORKER_CORE 7

#define RUNS 500000U

#define TEST_VECTOR 0xf2

// #define DUNE_SHINJUKU

static volatile int worker_ready, worker_stop;

static uint32_t tsc_sender[RUNS], timer_frequency[RUNS];
static uint32_t tsc_worker[RUNS], tsc_sender_worker[RUNS];
static uint32_t tsc_worker_worker[RUNS];

static uint64_t volatile sender_start_tsc, worker_start_tsc,
        worker_start_in_handler;

static volatile bool wait_worker = 1, handler_checkout;

static volatile uint32_t i_handler = 0;

static void
handler ( struct dune_tf *tf )
{
  static uint64_t last_now = 0;
  uint64_t now = _rdtsc ();

  tsc_sender_worker[i_handler] = now - sender_start_tsc;
  tsc_worker[i_handler] = now - worker_start_tsc;

  timer_frequency[i_handler] = now - last_now;
  last_now = now;

  i_handler++;

  dune_apic_eoi ();
  worker_start_in_handler = _rdtsc ();
  handler_checkout = 1;
}

void *
worker ( void *arg )
{
  pin_to_cpu ( WORKER_CORE );
  printf ( "Started worker thread on core %u\n", sched_getcpu () );

  volatile int ret = dune_enter ();
  if ( ret )
    {
      fprintf ( stderr, "failed to enter dune in thread 2\n" );
      exit ( 1 );
    }

#ifndef DUNE_SHINJUKU
  dune_apic_init_rt_entry ();
#endif
  dune_register_intr_handler ( TEST_VECTOR, handler );
  asm volatile ( "mfence" ::: "memory" );
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

#ifndef DUNE_SHINJUKU
      dune_apic_send_ipi ( TEST_VECTOR,
                           dune_apic_id_for_cpu ( WORKER_CORE, NULL ) );
#else
      dune_apic_send_posted_ipi ( TEST_VECTOR, WORKER_CORE );
#endif

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

  printf ( "Samples: %u\n"
           "Cycles by us: %u\n"
           "TSC overhead: %lu\n",
           RUNS,
           cycles_by_us,
           rdtsc_overhead );

  volatile int ret = dune_init_and_enter ();
  if ( ret )
    {
      fprintf ( stderr, "failed to initialize dune\n" );
      return ret;
    }

  dune_apic_init_rt_entry ();

  pthread_t tid;
  pthread_create ( &tid, NULL, worker, NULL );

  while ( !worker_ready )
    ;

  sender ();

  print ( tsc_sender, RUNS, "Sender", 0 );
  print ( tsc_sender_worker, RUNS, "Sender/Worker", 0 );
  print ( tsc_worker, RUNS, "Worker", 0 );
  print ( tsc_worker_worker, RUNS, "Worker/Worker", 0 );
  print ( timer_frequency, RUNS, "Timer frequency", 1 );

  pthread_join ( tid, NULL );
  printf ( "\nDUNE IPIs handled: %d\n", i_handler );

  return 0;
}
