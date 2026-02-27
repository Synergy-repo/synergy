
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

#include "../../tests/util.h"

#define CORE 0

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

#ifdef CONCORD_RDTSC
#define fake_work_ns fake_work_ns_rdtsc
#define fake_work_us fake_work_us_rdtsc
#endif

extern void
fake_work_ns ( unsigned );

static void
test_ns ( unsigned target )
{
  uint64_t cycles = _rdtsc ();
  fake_work_ns ( target );
  cycles = _rdtsc () - cycles;

  printf ( "  Fake work target: %u ns duration: %.2f us\n",
           target,
           cycles2us ( cycles ) );
}

int
main ( void )
{
  init_util ();
  pin_to_cpu ( CORE );

  puts ( "Test fake_work_ns:" );
  int i = 2;
  while(i--)
    {
      test_ns ( 500 );
      test_ns ( 1000 );
      test_ns ( 2500 );
      test_ns ( 10000 );
      test_ns ( 100000 );
      test_ns ( 500000 );
    }

  putchar ( '\n' );

  return 0;
}
