
#define _GNU_SOURCE
#include <sched.h>
#include <stdint.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "thread.h"
#include "timer.h"
#include "../util.h"

#define WORKER_CORE 5
#define TIMER_CORE 7

#define ITERATIONS_BY_US 420

static struct thread th_main, *th_worker;

volatile uint64_t count, interruptions;

void
worker ( void *arg )
{
  // while ( 1 )
  //  {
  count++;
  for ( volatile int i = 0; i < ITERATIONS_BY_US * 5; i++ )
    ;
  //  }
}

void
thread_return ( void )
{
  thread_set_main ( &th_main );
  __builtin_unreachable ();
}

void
handler ( int x )
{
  interruptions++;

  timer_received ();
  thread_swap2main ( th_worker, &th_main );
}

void *
worker_main ( void *arg )
{
  pin_to_cpu ( WORKER_CORE );
  th_worker = thread_create ( worker, NULL );
  assert ( th_worker );
  timer_init ( gettid () );

  int i = 1;
  while ( 1 )
    {
      // if ( interruptions >= 1 )
      //  break;

      // timer_enable ();

      thread_swap2worker ( &th_main, th_worker );
    }

  exit ( 0 );
}

int
main ( void )
{
  const struct sigaction sa = { .sa_handler = handler, .sa_flags = SA_NODEFER };
  sigaction ( SIGUSR1, &sa, NULL );

  pthread_t t;
  pthread_create ( &t, NULL, worker_main, NULL );

  pin_to_cpu ( TIMER_CORE );
  timer_main ();

  return 0;
}
