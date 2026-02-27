
// common utils for tests (benchmark)

#ifndef UTIL_H
#define UTIL_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <x86intrin.h>
#include <math.h>
#include <cpuid.h>

#define NS_PER_SEC 1000000000U

  /* return 1 if CPU has constant TCS increments */
  int
  check_constant_tsc ()
  {
    unsigned int eax, ebx, ecx, edx;

    // CPUID function 0x80000007: Advanced Power Management Information
    if ( __get_cpuid ( 0x80000007, &eax, &ebx, &ecx, &edx ) )
      {
        return ( edx & ( 1 << 8 ) ) !=
               0;  // Bit 8 of edx indicates constant TSC support
      }
    return 0;
  }

  // return amount cycles by second
  uint64_t
  util_get_tsc_freq ( void )
  {

    struct timespec sleeptime = { .tv_sec = 0,
                                  .tv_nsec = NS_PER_SEC / 2 }; /* 1/2 second */
    struct timespec t_start, t_end;

    uint64_t tsc_hz;

    if ( clock_gettime ( CLOCK_MONOTONIC_RAW, &t_start ) == 0 )
      {
        uint64_t ns, end, start = __rdtsc ();
        nanosleep ( &sleeptime, NULL );
        clock_gettime ( CLOCK_MONOTONIC_RAW, &t_end );
        end = __rdtsc ();
        ns = ( ( t_end.tv_sec - t_start.tv_sec ) * NS_PER_SEC );
        ns += ( t_end.tv_nsec - t_start.tv_nsec );

        double secs = ( double ) ns / NS_PER_SEC;
        tsc_hz = ( uint64_t ) ( ( end - start ) / secs );

        return tsc_hz;
      }

    return 0;
  }

  uint64_t
  my_rdtscp ()
  {
    uint64_t a, c, d;

    asm volatile ( "rdtscp" : "=a"( a ), "=d"( d ), "=c"( c ) );
    return a | ( d << 32 );
  }

  static void
  pin_to_cpu ( int cpu )
  {
    cpu_set_t cpus;
    CPU_ZERO ( &cpus );
    CPU_SET ( cpu, &cpus );
    if ( pthread_setaffinity_np ( pthread_self (), sizeof ( cpus ), &cpus ) )
      {
        printf ( "Could not pin thread to core %d.\n", cpu );
        exit ( 1 );
      }
  }

  static uint32_t cycles_by_us;

  static inline double
  cycles2ns ( uint32_t cycles )
  {
    return ( double ) cycles / ( cycles_by_us / ( double ) 1000.0 );
  }

  static inline double
  cycles2us ( uint32_t cycles )
  {
    return ( double ) cycles / cycles_by_us;
  }

  static int
  cmp_uint32 ( const void *a, const void *b )
  {
    uint32_t v1, v2;

    v1 = *( uint32_t * ) a;
    v2 = *( uint32_t * ) b;

    return ( v1 > v2 ) - ( v1 < v2 );
  }

  static inline uint32_t
  percentile ( uint32_t *buff, size_t len, float percentile )
  {
    unsigned int idx = len * percentile;
    return buff[idx];
  }

  static unsigned long rdtsc_overhead;

  static unsigned long
  measure_tsc_overhead ( void )
  {
    unsigned long t0, t1, overhead = ~0UL;

    for ( unsigned i = 0; i < 10000U; i++ )
      {
        t0 = _rdtsc ();
        asm volatile ( "" );
        t1 = _rdtsc ();
        if ( t1 - t0 < overhead )
          overhead = t1 - t0;

        // serialize cpu
        my_rdtscp ();
      }

    return overhead;
  }

  static uint32_t
  average ( uint32_t *buff, size_t size )
  {
    unsigned long long sum = 0;
    for ( size_t i = 0; i < size; i++ )
      sum += buff[i];

    return sum / size;
  }

  static double
  error_margin ( uint32_t *buff, size_t size )
  {
    uint64_t sum = 0;
    for ( size_t i = 0; i < size; i++ )
      sum += buff[i];

    double avg = ( double ) sum / size;

    uint64_t sum_p = 0;
    for ( size_t i = 0; i < size; i++ )
      sum_p += pow ( buff[i] - avg, 2 );

    double variance = sqrt ( sum_p / size );

    double Z = 1.96;
    double error = Z * ( variance / sqrt ( size ) );

    return error;
  }

  static inline uint32_t
  util_get_p50 ( uint32_t *buff, unsigned size, unsigned discard )
  {
    // ignore firsts 'discard' values
    buff += discard;
    size -= discard;

    qsort ( buff, size, sizeof ( *buff ), cmp_uint32 );
    return percentile ( buff, size, 0.50f );
  }

  static void
  print ( uint32_t *buff, size_t size, char *msg, uint32_t discard )
  {
    // ignore firsts 'discard' values
    buff += discard;
    size -= discard;

    qsort ( buff, size, sizeof ( *buff ), cmp_uint32 );

    // not remove rdtsc overhead
    // for ( size_t i = 0; i < size; i++ )
    //  buff[i] -= rdtsc_overhead;

    uint32_t min, p50, p75, p99, p999, max, avg;
    min = buff[0];
    p50 = percentile ( buff, size, 0.50f );
    p75 = percentile ( buff, size, 0.75f );
    p99 = percentile ( buff, size, 0.99f );
    p999 = percentile ( buff, size, 0.999f );
    max = buff[size - 1];
    avg = average ( buff, size );
    double margin = error_margin ( buff, size );

    printf ( "\n%s\n"
#ifdef STDDEV
             "  Err:   %.4lf (%.6f ns)\n"
#endif
             "  Avg:   %u (%.2f ns)\n"
             "  Min:   %u (%.2f ns)\n"
             "  50%%:   %u (%.2f ns)\n"
             "  75%%:   %u (%.2f ns)\n"
             "  99%%:   %u (%.2f ns)\n"
             "  99.9%%: %u (%.2f ns)\n"
             "  Max:   %u (%.2f ns)\n",
             msg,
#ifdef STDDEV
             margin,
             cycles2ns ( margin ),
#endif
             avg,
             cycles2ns ( avg ),
             min,
             cycles2ns ( min ),
             p50,
             cycles2ns ( p50 ),
             p75,
             cycles2ns ( p75 ),
             p99,
             cycles2ns ( p99 ),
             p999,
             cycles2ns ( p999 ),
             max,
             cycles2ns ( max ) );
  }

  static void
  print_tsc ( void )
  {
    printf ( "Cycles by us: %u\n"
             "TSC overhead: %lu\n",
             cycles_by_us,
             rdtsc_overhead );
  }

  static void
  init_util ( void )
  {
    cycles_by_us = util_get_tsc_freq () / 1000000UL;
    rdtsc_overhead = measure_tsc_overhead ();
  }

#ifdef __cplusplus
}
#endif

#pragma GCC diagnostic pop

#endif
