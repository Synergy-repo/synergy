
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

static void
warmup (void)
{
  calibrate (10000U);
}

static uint64_t
test_delay (unsigned iterations)
{
  uint64_t start, end, delay;
  unsigned int unsed;

  start = _rdtsc ();
  // work (iterations);
  calibrate (iterations);
  end = __rdtscp (&unsed);

  delay = (end - start - rdtsc_overhead) / cycles_by_us;

  return delay;
}

/* return number of interations by 1us */
static unsigned
calibrate (void)
{
  warmup ();

  unsigned iterations = 0;
  uint64_t delay_us;

  do
    {
      delay_us = test_delay (iterations++);
    }
  while (delay_us != 1);

  return iterations;
}

int
main (void)
{
  init_util ();
  pin_to_cpu (CORE);

  if (!check_constant_tsc ())
    {
      fprintf (stderr, "%s\n", "CPU not has constant TSC");
      exit (1);
    }

  printf ("%u\n", calibrate ());

  return 0;
}
