
#include "../util.h"
#include <bits/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <stdatomic.h>

#define RUNS 500000U

uint64_t tscs_start[RUNS];
uint64_t tscs_end[RUNS];
uint32_t tscs_delta[RUNS];

static atomic_int synergy_timer_status;

static __attribute__ ( ( noinline ) ) void
synergy_timer_enable ( atomic_int *t )
{
  atomic_store ( t, 1 );
  asm volatile ( "" );
}

static __attribute__ ( ( noinline ) ) void
synergy_timer_disable ( atomic_int *t )
{
  atomic_store ( t, 0 );
  asm volatile ( "" );
}

void
test_setittimer ( void )
{
  for ( int i = 0; i < RUNS; i++ )
    {
      tscs_start[i] = rdtsc ();
      // single-shot timer to starrint in 60 secs (never run)
      setitimer ( ITIMER_REAL,
                  &( struct itimerval ){ .it_value.tv_sec = 60 },
                  NULL );
      asm volatile ( "" );

      // disable timer
      setitimer ( ITIMER_REAL, &( struct itimerval ){ 0 }, NULL );

      tscs_end[i] = rdtscp ();
    }

  for ( int i = 0; i < RUNS; i++ )
    tscs_delta[i] = tscs_end[i] - tscs_start[i];

  print ( tscs_delta, RUNS, "setitimer", 0 );
}

void
test_timer_create ( void )
{
  timer_t timer_id;
  // create default time options
  if ( timer_create ( CLOCK_MONOTONIC, NULL, &timer_id ) )
    {
      fprintf ( stderr, "Error time_create" );
      exit ( 1 );
    }

  for ( int i = 0; i < RUNS; i++ )
    {
      tscs_start[i] = rdtsc ();
      // single-shot timer to starrint in 60 secs (never run)
      timer_settime ( timer_id,
                      0,
                      &( struct itimerspec ){ .it_value.tv_sec = 60 },
                      NULL );
      // disable timer
      timer_settime ( timer_id, 0, &( struct itimerspec ){ 0 }, NULL );

      tscs_end[i] = rdtscp ();
    }

  for ( int i = 0; i < RUNS; i++ )
    tscs_delta[i] = tscs_end[i] - tscs_start[i];

  print ( tscs_delta, RUNS, "timer_create", 0 );
}

void
test_synergy_timer ( void )
{
  for ( int i = 0; i < RUNS; i++ )
    {
      tscs_start[i] = rdtsc ();
      // single-shot timer to starrint in 60 secs (never run)
      synergy_timer_enable ( &synergy_timer_status );

      synergy_timer_disable ( &synergy_timer_status );

      tscs_end[i] = rdtscp ();
    }

  for ( int i = 0; i < RUNS; i++ )
    tscs_delta[i] = tscs_end[i] - tscs_start[i];

  print ( tscs_delta, RUNS, "synergy_timer", 0 );
}

void
test_alarm ( void )
{
  for ( int i = 0; i < RUNS; i++ )
    {
      tscs_start[i] = rdtsc ();
      // single-shot timer to starrint in 60 secs (never run)
      alarm ( 60 );

      // disable timer
      alarm ( 0 );

      tscs_end[i] = rdtscp ();
    }

  for ( int i = 0; i < RUNS; i++ )
    tscs_delta[i] = tscs_end[i] - tscs_start[i];

  print ( tscs_delta, RUNS, "alarm", 0 );
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

  test_setittimer ();
  test_timer_create ();
  test_synergy_timer ();
  test_alarm ();
}
