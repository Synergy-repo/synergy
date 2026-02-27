
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

#include "util.h"

#define RUNS 5000000U
#define CORE 1

#ifdef CONCORD
#define CONCORD_SKIP __attribute__ ( ( annotate ( "concord_skip" ) ) )
void
concord_func ( void )
{
  return;
}
int concord_preempt_now;
#else
#define CONCORD_SKIP
#endif

#define X5( x ) x x x x x
#define X10( x ) X5 ( x ) X5 ( x )
#define X50( x ) X10 ( x ) X10 ( x ) X10 ( x ) X10 ( x ) X10 ( x )
#define X100( x ) X50 ( x ) X50 ( x )
#define X200( x ) X100 ( x ) X100 ( x )

static unsigned iterations_by_us;

static __attribute__ ( ( noinline ) ) void
fake_work_ns ( unsigned target_ns )
{
  unsigned end = target_ns * iterations_by_us / 1000;
  for ( unsigned i = 0; i < end; i++ )
    {
      X200 ( asm volatile( "nop" ); )
    }
}

static void
fake_work_us ( unsigned target_us )
{
  fake_work_ns ( target_us * 1000 );
}

static CONCORD_SKIP __attribute__ ( ( noinline ) ) void
calibration ( void )
{
  for ( unsigned i = 0; i < RUNS; i++ )
    {
      X200 ( asm volatile( "nop" ); )
    }
}

static void
test ( unsigned target )
{
  uint64_t cycles = __rdtsc ();
  fake_work_us ( target );
  cycles = __rdtsc () - cycles;

  printf ( "  Fake work target: %u us duration: %.2f us\n",
           target,
           cycles2us ( cycles ) );
}

static void
test_ns ( unsigned target )
{
  uint64_t cycles = __rdtsc ();
  fake_work_ns ( target );
  cycles = __rdtsc () - cycles;

  printf ( "  Fake work target: %u ns duration: %.2f us\n",
           target,
           cycles2us ( cycles ) );
}

int
main ( void )
{
  init_util ();
  pin_to_cpu ( CORE );

#define AVG 500

  double avg_cycles_by_iteration = 0;
  for ( int i = 0; i < AVG; i++ )
    {
      uint64_t cycles = _rdtsc ();
      calibration ();
      cycles = _rdtsc () - cycles;

      avg_cycles_by_iteration += cycles / ( double ) RUNS;
    }

  avg_cycles_by_iteration /= AVG;

  iterations_by_us = cycles_by_us / avg_cycles_by_iteration;

  printf ( "Calibration:\n"
           "  Cycles by us: %u\n"
           "  Cycles by iteration: %.2lf\n"
           "  Iterations by us: %u\n",
           cycles_by_us,
           avg_cycles_by_iteration,
           iterations_by_us );

  puts ( "\nTest fake_work:" );
  test ( 1 );
  test ( 5 );
  test ( 20 );
  test ( 100 );
  test ( 500 );

  puts ( "\nTest fake_work_ns:" );
  test_ns ( 500 );
  test_ns ( 1500 );
  test_ns ( 2000 );
  test_ns ( 100000 );

  return 0;
}
