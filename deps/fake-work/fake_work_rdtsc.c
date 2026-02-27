
#ifndef CALIBRATE
#if ( ITERATIONS_BY_US + 0 ) == 0
#error "ITERATIONS_BY_US not defined"
#endif
#else
// only to compile
#define ITERATIONS_BY_US 1
#endif

#include <stdint.h>

#include "work.h"

void
fake_work_ns_rdtsc ( unsigned target_ns )
{
  work ( target_ns * ( ITERATIONS_BY_US / ( double ) 1000 ) );
}

void
fake_work_us_rdtsc ( unsigned target_us )
{
  work ( target_us * ITERATIONS_BY_US );
}

#ifdef CALIBRATE
void
calibrate_work ( unsigned i )
{
  work ( i );
}
#endif
