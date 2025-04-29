
// clang-format off
#define X10( x ) x x x x x x x x x x
#define X100( x ) \
  X10( x ) X10( x ) X10( x ) X10( x ) X10( x ) \
  X10( x ) X10( x ) X10( x ) X10( x ) X10( x )
#define X300( x ) X100(x) X100(x) X100(x)
// clang-format on

__attribute__ ( ( noinline ) ) static void
work ( unsigned long i )
{
  while ( i-- )
    {
      X300 ( asm volatile( "nop" ); )
    }
}

#ifndef CALIBRATE

#ifdef CONCORD_RDTSC
#define fake_work_ns fake_work_ns_rdtsc
#endif

void
fake_work_ns ( unsigned target_ns )
{
  switch ( target_ns )
    {
      case 500:
        work ( ITERATIONS_500NS );
        break;
      case 1000:
        work ( ITERATIONS_1US );
        break;
      case 2500:
        work ( ITERATIONS_2500NS );
        break;
      case 10000:
        work ( ITERATIONS_10US );
        break;
      case 100000:
        work ( ITERATIONS_100US );
        break;
      case 500000:
        work ( ITERATIONS_500US );
        break;
    }
}

#else
void
calibrate_work ( unsigned i )
{
  work ( i );
}
#endif
