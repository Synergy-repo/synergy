
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "../tests/util.h"
#include "ipi.h"

static volatile int worker_ready, worker_stop;

#define MAIN_CORE 0
#define TOT_WORKERS 1

static int workers_cores[] = { 2, 4 };

static long values[16];

/* not allocate any thing inside this function to avoid compiler stack
 * allocation (i.e rsp decrement )*/
void
test_red_zone ( void )
{
  asm volatile ( "movq $42, -8(%rsp)\n"    // Store 42 at 8 bytes below RSP
                 "movq $42, -16(%rsp)\n"   // Store 42 at 16 bytes below RSP
                 "movq $42, -24(%rsp)\n"   // Store 42 at 24 bytes below RSP
                 "movq $42, -32(%rsp)\n"   // Store 42 at 32 bytes below RSP
                 "movq $42, -40(%rsp)\n"   // Store 42 at 40 bytes below RSP
                 "movq $42, -48(%rsp)\n"   // Store 42 at 48 bytes below RSP
                 "movq $42, -56(%rsp)\n"   // Store 42 at 56 bytes below RSP
                 "movq $42, -64(%rsp)\n"   // Store 42 at 64 bytes below RSP
                 "movq $42, -72(%rsp)\n"   // Store 42 at 72 bytes below RSP
                 "movq $42, -80(%rsp)\n"   // Store 42 at 80 bytes below RSP
                 "movq $42, -88(%rsp)\n"   // Store 42 at 88 bytes below RSP
                 "movq $42, -96(%rsp)\n"   // Store 42 at 96 bytes below RSP
                 "movq $42, -104(%rsp)\n"  // Store 42 at 104 bytes below RSP
                 "movq $42, -112(%rsp)\n"  // Store 42 at 112 bytes below RSP
                 "movq $42, -120(%rsp)\n"  // Store 42 at 120 bytes below RSP
                 "movq $42, -128(%rsp)\n"  // Store 42 at 128 bytes below RSP
  );

  worker_ready++;
  while ( !worker_stop )
    ;

  asm volatile ( "movq -8(%%rsp), %%rax\n"
                 "movq %%rax, %0\n"
                 "movq -16(%%rsp), %%rax\n"
                 "movq %%rax, %1\n"
                 "movq -24(%%rsp), %%rax\n"
                 "movq %%rax, %2\n"
                 "movq -32(%%rsp), %%rax\n"
                 "movq %%rax, %3\n"
                 "movq -40(%%rsp), %%rax\n"
                 "movq %%rax, %4\n"
                 "movq -48(%%rsp), %%rax\n"
                 "movq %%rax, %5\n"
                 "movq -56(%%rsp), %%rax\n"
                 "movq %%rax, %6\n"
                 "movq -64(%%rsp), %%rax\n"
                 "movq %%rax, %7\n"
                 "movq -72(%%rsp), %%rax\n"
                 "movq %%rax, %8\n"
                 "movq -80(%%rsp), %%rax\n"
                 "movq %%rax, %9\n"
                 "movq -88(%%rsp), %%rax\n"
                 "movq %%rax, %10\n"
                 "movq -96(%%rsp), %%rax\n"
                 "movq %%rax, %11\n"
                 "movq -104(%%rsp), %%rax\n"
                 "movq %%rax, %12\n"
                 "movq -112(%%rsp), %%rax\n"
                 "movq %%rax, %13\n"
                 "movq -120(%%rsp), %%rax\n"
                 "movq %%rax, %14\n"
                 "movq -128(%%rsp), %%rax\n"
                 "movq %%rax, %15\n"
                 : "=m"( values[0] ),
                   "=m"( values[1] ),
                   "=m"( values[2] ),
                   "=m"( values[3] ),
                   "=m"( values[4] ),
                   "=m"( values[5] ),
                   "=m"( values[6] ),
                   "=m"( values[7] ),
                   "=m"( values[8] ),
                   "=m"( values[9] ),
                   "=m"( values[10] ),
                   "=m"( values[11] ),
                   "=m"( values[12] ),
                   "=m"( values[13] ),
                   "=m"( values[14] ),
                   "=m"( values[15] )

                 :
                 : "memory" );
}

void
test_red_zone2 ( void )
{
  int i, err = 0;
  for ( i = 0; i < 16; i++ )
    {
      if ( values[i] != 42 )
        {
          err = 1;
          printf ( "Red zone corruption at offset -%d: expected 42, got %ld\n",
                   ( i + 1 ) * 8,
                   values[i] );
        }
    }
  if ( !err )
    puts ( "Red zone OK" );
}

static void
my_handler ( void )
{
  puts ( "Uhul from handler" );
  // clang-format off
  asm volatile( "mov $3, %%rax;"
                "mov $3, %%rdi;"
                "mov $3, %%rsi;"
                "mov $3, %%rdx;"
                "mov $3, %%rcx;"
                "mov $3, %%r8;"
                "mov $3, %%r9;"
                "mov $3, %%r10;"
                "mov $3, %%r11;"
                "mov $3, %%rbx;"
                "mov $3, %%r12;"
                "mov $3, %%r13;"
                "mov $3, %%r14;"
                "mov $3, %%r15;"
                :
                :
                : "%rax",
                  "%rdi",
                  "%rsi",
                  "%rdx",
                  "%rcx",
                  "%r8",
                  "%r9",
                  "%r10",
                  "%r11",
                  "%rbx",
                  "%r12",
                  "%r13",
                  "%r14",
                  "%r15" );
  // clang-format on

  worker_stop = 1;
}

void *
worker ( void *arg )
{
  ipi_init_worker ();

  // register int rax asm( "rax" ) = 42;
  asm volatile ( "mov $5, %%rax\n\t;"
                 "mov $5, %%rdi\n\t;"
                 "mov $5, %%rsi\n\t;"
                 "mov $5, %%rdx\n\t;"
                 "mov $5, %%rcx\n\t;"
                 "mov $5, %%r8\n\t;"
                 "mov $5, %%r9\n\t;"
                 "mov $5, %%r10\n\t;"
                 "mov $5, %%r11\n\t;"
                 "mov $5, %%rbx\n\t;"
                 "mov $5, %%r12\n\t;"
                 "mov $5, %%r13\n\t;"
                 "mov $5, %%r14\n\t;"
                 "mov $5, %%r15\n\t;"
                 :
                 :
                 : );

  // test_red_zone ();
  // test_red_zone2 ();

  worker_ready += 1;
  while ( !worker_stop )
    {
      char buff[512];
      // read ( 1, buff, sizeof buff );
    }

  puts ( "Uhul from worker" );

  return NULL;
}

static pthread_t
setup_worker ( int core )
{
  cpu_set_t cpus;
  CPU_ZERO ( &cpus );
  CPU_SET ( core, &cpus );
  pthread_attr_t attr;
  pthread_attr_init ( &attr );
  pthread_attr_setaffinity_np ( &attr, sizeof ( cpu_set_t ), &cpus );
  pthread_t tid;
  assert ( 0 == pthread_create ( &tid, &attr, worker, NULL ) );
  return tid;
}

static void
test_single_ipi ( void )
{
  worker_ready = worker_stop = 0;

  /* setup worker */
  pthread_t tids[TOT_WORKERS];
  for ( int i = 0; i < TOT_WORKERS; i++ )
    {
      tids[i] = setup_worker ( workers_cores[i] );
    }

  while ( worker_ready != TOT_WORKERS )
    ;

  /* send IPI */
  for ( int i = 0; i < TOT_WORKERS; i++ )
    {
      ipi_send_interrupt ( workers_cores[i] );
    }

  for ( int i = 0; i < TOT_WORKERS; i++ )
    {
      pthread_join ( tids[i], NULL );
    }
}

static void
test_multcast_ipi ( void )
{
  worker_ready = worker_stop = 0;

  /* setup worker */
  pthread_t tids[TOT_WORKERS];
  for ( int i = 0; i < TOT_WORKERS; i++ )
    {
      tids[i] = setup_worker ( workers_cores[i] );
    }

  while ( worker_ready != TOT_WORKERS )
    ;

  /* send IPI */
  struct ipi_cpumask mask = { 0 };
  for ( int i = 0; i < TOT_WORKERS; i++ )
    {
      ipi_cpumask_add ( workers_cores[i], mask );
    }

  ipi_send_interrupt_many ( mask );
  ipi_cpumask_clear ( mask );

  for ( int i = 0; i < TOT_WORKERS; i++ )
    {
      pthread_join ( tids[i], NULL );
    }
}

int
main ( void )
{
  init_util ();
  cpu_set_t cpus;

  /* setup main thread */
  pin_to_cpu ( MAIN_CORE );

  /* setup trap lib */
  assert ( ipi_init ( my_handler ) == 0 );

  puts ( "Testing single IPI" );
  test_single_ipi ();

  // puts ( "Testing multcast IPI" );
  // test_multcast_ipi ();

  return 0;
}
