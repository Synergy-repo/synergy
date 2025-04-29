
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
#include "trap.h"

// ensure not using hyperthreads of same worker core
#define SENDER_CORE 2

#define RUNS 500000

#define WORKERS 4
static int workers_core[WORKERS] = { 4, 5, 6, 7 };

static volatile int worker_ready, worker_stop;
static uint32_t tsc_sender[RUNS];

static __thread uint32_t tsc_worker[RUNS];
static __thread uint64_t volatile worker_start_tsc;
static __thread int worker_id;
static __thread volatile bool handler_checkout;

static volatile bool wait_workers[WORKERS];

void
jmp_for_me ( void )
{
  static __thread uint32_t i_handler;

  uint64_t now = __rdtsc ();
  tsc_worker[i_handler] = now - worker_start_tsc;

  i_handler++;
  handler_checkout = 1;
}

void *
worker ( void *arg )
{
  worker_id = ( int ) ( uintptr_t ) arg;
  pin_to_cpu ( workers_core[worker_id] );
  printf ( "Started worker thread on core %u\n", sched_getcpu () );

  /* init after pin_to_cpu */
  trap_init_worker ();
  wait_workers[worker_id] = true;

  worker_ready++;
  while ( !worker_stop )
    {
      worker_start_tsc = __rdtsc ();
      if ( handler_checkout )
        {
          handler_checkout = 0;
          wait_workers[worker_id] = 0;
        }
    }

  char msg[16];
  snprintf ( msg, sizeof msg, "Worker %u", worker_id );
  print ( tsc_worker, RUNS, msg, 0 );

  return NULL;
}

static void
sender ( void )
{
  pin_to_cpu ( SENDER_CORE );
  printf ( "Started sender thread on core %u\n", sched_getcpu () );

  uint64_t start;

  for ( unsigned int volatile i = 0; i < RUNS; i++ )
    {
      start = __rdtsc ();

      for ( int j = 0; j < WORKERS; j++ )
        {
          int r = trap_send_interrupt ( workers_core[j] );

          assert ( r == 0 );

          while ( wait_workers[j] )
            ;

          wait_workers[j] = 1;
        }

      tsc_sender[i] = __rdtsc () - start;
    }

  worker_stop = 1;

  print ( tsc_sender, RUNS, "Sender", 0 );
}

int
main ( int argc, char **argv )
{
  init_util ();

  printf ( "Samples: %u\n"
           "Cycles by us: %u\n",
           RUNS,
           cycles_by_us );

  if ( trap_init ( jmp_for_me ) < 0 )
    return 1;

  pthread_t tids[WORKERS];
  for ( int i = 0; i < WORKERS; i++ )
    pthread_create ( &tids[i], NULL, worker, ( void * ) ( uintptr_t ) i );

  while ( worker_ready != WORKERS )
    ;

  sender ();

  for ( int i = 0; i < WORKERS; i++ )
    pthread_join ( tids[i], NULL );

  trap_free ();

  return 0;
}
