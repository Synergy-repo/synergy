
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

#include "instrumentation_loop.h"
#include "util.h"

int __thread concord_preempt_now;
void
concord_func ( void )
{
}

int
main ( int argc, char **argv )
{
  uint64_t iterations = atoi ( argv[1] );

#define CORE 0

  // init_util ();
  pin_to_cpu ( CORE );

  uint64_t start_instrumented = _rdtsc ();
  instrumented_loop ( iterations );
  uint64_t end_instrumented = _rdtsc ();

  asm volatile( "mfence" ::: "memory" );

  uint64_t start_simple = _rdtsc ();
  simple_loop ( iterations );
  uint64_t end_simple = _rdtsc ();

  uint64_t simple_delta, instrumented_delta;

  simple_delta = end_simple - start_simple;
  instrumented_delta = end_instrumented - start_instrumented;

  printf ( "Simple loop:\n"
           "  ticks: %lu\n"
           "  ticks per iteration: %.4f\n",
           simple_delta,
           simple_delta / ( double ) iterations );

  printf ( "Instrumentated loop:\n"
           "  ticks: %lu\n"
           "  ticks per iteration: %.4f\n",
           instrumented_delta,
           instrumented_delta / ( double ) iterations );
}
