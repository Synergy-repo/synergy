
#include "instrumentation_loop.h"

CONCORD_SKIP __attribute__ ( ( noinline ) ) void
simple_loop ( unsigned iterations )
{
  for ( unsigned i = 0; i < iterations; i++ )
    asm volatile( "nop" );
}

__attribute__ ( ( noinline ) ) void
instrumented_loop ( unsigned iterations )
{
  for ( unsigned i = 0; i < iterations; i++ )
    asm volatile( "nop" );
}
