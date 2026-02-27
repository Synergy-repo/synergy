
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#include <unistd.h>
#include <signal.h>
#include <x86intrin.h>

#include "util.h"

#ifndef __NR_uintr_register_handler
#define __NR_uintr_register_handler 471
#define __NR_uintr_unregister_handler 472
#define __NR_uintr_create_fd 473
#define __NR_uintr_register_sender 474
#define __NR_uintr_unregister_sender 475
#define __NR_uintr_wait 476
#endif

#define uintr_register_handler( handler, flags ) \
  syscall ( __NR_uintr_register_handler, handler, flags )
#define uintr_unregister_handler( flags ) \
  syscall ( __NR_uintr_unregister_handler, flags )
#define uintr_create_fd( vector, flags ) \
  syscall ( __NR_uintr_create_fd, vector, flags )
#define uintr_register_sender( fd, flags ) \
  syscall ( __NR_uintr_register_sender, fd, flags )
#define uintr_unregister_sender( ipi_idx, flags ) \
  syscall ( __NR_uintr_unregister_sender, ipi_idx, flags )
#define uintr_wait( flags ) syscall ( __NR_uintr_wait, flags )

#define SENDER_CORE 2
#define WORKER_CORE 4

#define RUNS 500000U

static volatile int worker_ready, worker_stop;

static uint32_t tsc_sender[RUNS], timer_frequency[RUNS];
static uint32_t tsc_worker_worker[RUNS];
uint32_t tsc_worker[RUNS], tsc_sender_worker[RUNS];

uint64_t volatile sender_start_tsc, worker_start_tsc, worker_start_in_handler;

static volatile int wait_worker = 1;

uint32_t handler_checkout;

volatile uint32_t i_handler = 0;

static int worker_fd;

//__attribute__ ( ( interrupt ) ) void
// handler ( void *, long x )
__attribute__ ( ( target ( "general-regs-only" ) ) ) void
handler ( void )
{
  static uint64_t last_now = 0;
  uint64_t now = _rdtsc ();

  tsc_sender_worker[i_handler] = now - sender_start_tsc;
  tsc_worker[i_handler] = now - worker_start_tsc;

  timer_frequency[i_handler] = now - last_now;
  last_now = now;

  i_handler++;
  worker_start_in_handler = _rdtsc ();
  handler_checkout = 1;
}

__attribute__ ( ( naked ) ) void
ui_handler ()
{
  asm volatile ( "push %%rax\n\t"
                 "push %%rbx\n\t"
                 "push %%rcx\n\t"
                 "push %%rdx\n\t"
                 "push %%rsi\n\t"
                 "push %%rdi\n\t"
                 "push %%rbp\n\t"
                 "push %%r8\n\t"
                 "push %%r9\n\t"
                 "push %%r10\n\t"
                 "push %%r11\n\t"
                 "push %%r12\n\t"
                 "push %%r13\n\t"
                 "push %%r14\n\t"
                 "push %%r15\n\t"
                 :
                 :
                 : "memory" );
  handler ();

  asm volatile ( "pop %%r15\n\t"
                 "pop %%r14\n\t"
                 "pop %%r13\n\t"
                 "pop %%r12\n\t"
                 "pop %%r11\n\t"
                 "pop %%r10\n\t"
                 "pop %%r9\n\t"
                 "pop %%r8\n\t"
                 "pop %%rbp\n\t"
                 "pop %%rdi\n\t"
                 "pop %%rsi\n\t"
                 "pop %%rdx\n\t"
                 "pop %%rcx\n\t"
                 "pop %%rbx\n\t"
                 "pop %%rax\n\t"
                 "add $8, %%rsp\n\t"
                 "uiret\n\t"
                 :
                 :
                 : "memory" );
}

static void *
worker ( void *arg )
{
  pin_to_cpu ( WORKER_CORE );
  printf ( "Started worker thread on core %u\n", sched_getcpu () );

  if ( uintr_register_handler ( ui_handler, 0 ) )
    {
      fprintf ( stderr, "%s\n", "Error uintr_register_handler" );
      exit ( 1 );
    }

  worker_fd = uintr_create_fd ( 0, 0 );
  if ( worker_fd < 0 )
    {
      fprintf ( stderr, "%s\n", "Error on uintr_create_fd" );
      exit ( 1 );
    }

  _stui ();

  uint32_t i = 0;

  worker_ready = 1;
  while ( !worker_stop )
    {
      worker_start_tsc = _rdtsc ();
      if ( handler_checkout )
        {
          tsc_worker_worker[i++] = _rdtsc () - worker_start_in_handler;
          handler_checkout = 0;
          wait_worker = 0;
        }
    }

  return NULL;
}

static void
sender ( void )
{
  pin_to_cpu ( SENDER_CORE );
  printf ( "Started sender thread on core %u\n", sched_getcpu () );

  int worker_uipi_index = uintr_register_sender ( worker_fd, 0 );
  if ( worker_uipi_index < 0 )
    {
      fprintf ( stderr, "%s\n", "Error uintr_register_sender" );
      exit ( 1 );
    }

  for ( unsigned int i = 0; i < RUNS; i++ )
    {
      sender_start_tsc = _rdtsc ();

      _senduipi ( worker_uipi_index );

      tsc_sender[i] = _rdtsc () - sender_start_tsc;

      while ( wait_worker )
        ;

      wait_worker = 1;
    }

  worker_stop = 1;
}

int
main ( int argc, char **argv )
{
  init_util ();

  printf ( "Samples: %u\n"
           "Cycles by us: %u\n"
           "TSC overhead: %lu\n",
           RUNS,
           cycles_by_us,
           rdtsc_overhead );

  pthread_t tid;
  pthread_create ( &tid, NULL, worker, NULL );

  while ( !worker_ready )
    ;

  sender ();
  pthread_join ( tid, NULL );

  print ( tsc_sender, RUNS, "Sender (Time to call 'send')", 0 );
  print ( tsc_sender_worker,
          RUNS,
          "Sender/Worker (Time between call 'send' and the interrupt arrive on "
          "receiver)",
          0 );
  print ( tsc_worker,
          RUNS,
          "Worker (Time to receiver of interrupt change from main flow to "
          "interrupt flow)",
          0 );
  print ( tsc_worker_worker,
          RUNS,
          "Worker/Worker (Time to receiver of interrupt change go back from "
          "interrupt flow to main flow)",
          0 );
  print ( timer_frequency, RUNS, "Timer frequency", 1 );

  printf ( "\nInterrupts handled: %d\n", i_handler );

  return 0;
}
