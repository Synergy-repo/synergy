
#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* trap frame */
struct thread_tf
{
  /* argument registers, can be clobbered by callee */
  uint64_t rdi; /* first argument */
  uint64_t rsi;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;

  /* callee-saved registers */
  uint64_t rbx;
  uint64_t rbp;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;

  /* special-purpose registers */
  uint64_t rax; /* holds return value */
  uint64_t rip; /* instruction pointer */
  uint64_t rsp; /* stack pointer */
};

struct thread
{
  struct thread_tf tf;
  uint64_t *stack;
};

/* in bytes */
#define STACK_SIZE ( 128 * 1024 )

typedef void ( *thread_func ) ( void *arg );

static inline struct thread *
thread_alloc ( void )
{

  void *mem = malloc ( sizeof ( struct thread ) + STACK_SIZE );
  if ( !mem )
    return NULL;

  struct thread *th;

  th = mem;
  th->stack =
          ( uint64_t * ) ( ( ( uint64_t ) mem ) + sizeof ( struct thread ) );

  // debug
  memset ( th->stack, 1, STACK_SIZE );

  return th;
}

static inline void
thread_free ( struct thread *th )
{
  free ( th );
}

extern void
thread_return ( void );

static void
thread_setup_tf ( struct thread *th, thread_func fn, void *arg )
{
  /* go to top of the stack */
  uint64_t *rsp = ( uint64_t * ) ( ( uint64_t ) th->stack + STACK_SIZE );
  rsp -= 1;

  /* align stack in 16 bytes (four LSB set to 0) according to
   * System V Application Binary Interface (section 3.4.1) */
  rsp = ( uint64_t * ) ( ( ( uint64_t ) rsp ) & ~0x0F );

  /* set thread return address */
  *rsp = ( uint64_t ) thread_return;

  th->tf.rsp = ( uint64_t ) rsp;
  th->tf.rip = ( uint64_t ) fn;
  th->tf.rdi = ( uint64_t ) arg;
}

static struct thread *
thread_create ( thread_func fn, void *arg )
{

  struct thread *th = thread_alloc ();
  if ( !th )
    return NULL;

  thread_setup_tf ( th, fn, arg );

  return th;
}

/* in thread_swap.S */

extern void
thread_set_main ( struct thread *main );

extern void
thread_swap2worker ( struct thread *main, struct thread *worker );

extern void
thread_swap2main ( struct thread *worker, struct thread *main );

#endif
