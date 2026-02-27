
/* ensure that cpu frequency scalling is disabled  before run this */

#define _GNU_SOURCE
#include <assert.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

#include "../../tests/util.h"
//#include "work.h"

#define CORE 0

extern void
calibrate_work ( unsigned );

__thread int concord_preempt_now;
__thread int concord_start_time;
__thread int concord_preempt_after_cycle;
void
concord_func ( void )
{
}

void
concord_rdtsc_func ( void )
{
}

static void
warmup ( void )
{
  calibrate_work ( 10000U );
}

static uint64_t
test_delay ( unsigned iterations )
{
  uint64_t start, end, delay;
  unsigned int unsed;

  start = _rdtsc ();
  calibrate_work ( iterations );
  end = __rdtscp ( &unsed );

  delay = ( end - start - rdtsc_overhead ) / cycles_by_us;

  return delay;
}

/* return number of interations by 1us */
static unsigned
calibrate ( unsigned target_us )
{
  warmup ();

  unsigned iterations, max_iterations = 0;
  uint64_t delay_us;

  for ( unsigned i = 0; i < 5; ++i )
    {
      iterations = 0;
      do
        {
          iterations++;
          delay_us = test_delay ( iterations );
        }
      while ( delay_us != target_us );

      max_iterations =
              ( iterations > max_iterations ) ? iterations : max_iterations;
    }

  return max_iterations;
}

int
main ( void )
{
  init_util ();
  pin_to_cpu ( CORE );

  if ( !check_constant_tsc () )
    {
      fprintf ( stderr, "%s\n", "CPU not has constant TSC" );
      exit ( 1 );
    }

  unsigned us1 = calibrate ( 1 );
  unsigned us2 = calibrate ( 2 );
  unsigned us10 = calibrate ( 10 );
  unsigned us100 = calibrate ( 100 );
  unsigned us500 = calibrate ( 500 );

  printf ( "-DITERATIONS_500NS=%u -DITERATIONS_1US=%u "
           "-DITERATIONS_2500NS=%u -DITERATIONS_10US=%u "
           "-DITERATIONS_100US=%u -DITERATIONS_500US=%u",
           us1 / 2,
           us1,
           us2 + us1 / 2,
           us10,
           us100,
           us500 );

  return 0;
}
