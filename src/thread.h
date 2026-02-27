
#ifndef THREAD_H
#define THREAD_H

/* User level thread */

#include <stdint.h>

#include <rte_mempool.h>
#include <rte_branch_prediction.h>

#include "synergy_internal.h"
#include "compiler.h"

#define CANARY 0x00000AFF

/* trap frame, this layout should match with thread_switch.S */
struct thread_tf
{
  /* callee-saved registers */
  uint64_t rbx;
  uint64_t rbp;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;

  /* special-purpose registers */
  uint64_t rip; /* instruction pointer */
  uint64_t rsp; /* stack pointer */
} __aligned ( CACHE_LINE_SIZE );

static_assert ( ( STACK_SIZE % sizeof ( uint64_t ) ) == 0,
                "User level stack size is not multiple of QWORD (8 bytes)" );

#define BYTES_TO_QWORDS( x ) ( ( x ) / sizeof ( uint64_t ) )

struct thread
{
  struct thread_tf tf; /* should be first */
  /* base stack pointer */
  uint64_t stack[BYTES_TO_QWORDS ( STACK_SIZE )] __aligned ( 8 );
#ifdef THREAD_DEBUG
  uint64_t wait_start;
  uint32_t wait_time;
  uint32_t preempt_count;
#endif
};

typedef void ( *thread_func ) ( void );

extern struct rte_mempool *thread_pool;

extern thread_func fn_start;
extern void *dfl_fn_exit;

static inline bool
thread_alloc_bulk ( struct thread **threads, unsigned size )
{
  return 0 == rte_mempool_get_bulk ( thread_pool, ( void ** ) threads, size );
}

static inline void
thread_free ( struct thread *th )
{
  rte_mempool_put ( thread_pool, th );
}

static inline void
thread_free_bulk ( struct thread **threads, unsigned size )
{
  rte_mempool_put_bulk ( thread_pool, ( void ** ) threads, size );
}

static uint64_t *
thread_setup_return ( struct thread *th, void *fn_exit )
{
  assert ( *( th->stack ) == CANARY );
  uint64_t *rsp;

  /* go to top of the stack */
  // uint64_t *rsp = ( uint64_t * ) ( ( uint64_t ) th->stack + STACK_SIZE );
  rsp = &th->stack[BYTES_TO_QWORDS ( STACK_SIZE ) - 1];

  /* Once control has been transferred to the function entry point, i.e.
   * immediately after the return address has been pushed, %rsp points to the
   * return address, and the value of (%rsp + 8) is a multiple of 16 (32 or
   * 64).(System V ABI 3.2.2) */
  rsp = ( uint64_t * ) ( ( ( uint64_t ) rsp ) & ~0x0F );

  rsp--;
  /* set thread return address */
  *rsp = ( uint64_t ) fn_exit;

  assert ( ( ( uint64_t ) rsp + 8 ) % 16 == 0 );
  return rsp;
}

static void
thread_setup_tf ( struct thread *th, thread_func fn, void *fn_exit )
{
  uint64_t *rsp = thread_setup_return ( th, fn_exit );
  th->tf.rsp = ( uint64_t ) rsp;
  th->tf.rip = ( uint64_t ) fn;
  // th->tf.rdi = ( uint64_t ) arg;
  th->tf.rbp = 0;
}

// static inline bool
// thread_create_bulk ( struct thread **threads, void **args, unsigned int size
// )
//{
//  if ( unlikely ( !thread_alloc_bulk ( threads, size ) ) )
//    return false;
//
//  for ( unsigned i = 0; i < size; i++ )
//    {
//      struct thread *th = threads[i];
//#ifdef THREAD_DEBUG
//      th->preempt_count = 0;
//      th->wait_time = 0;
//#endif
//      thread_setup_tf ( th, fn_start, args[i], dfl_fn_exit );
//    }
//
//  return true;
//}

static inline struct thread *
thread_create ( thread_func fn, void *fn_exit )
{

  struct thread *th;
  if ( unlikely ( !thread_alloc_bulk ( &th, 1 ) ) )
    return NULL;

#ifdef THREAD_DEBUG
  th->preempt_count = 0;
#endif

  thread_setup_tf ( th, fn, fn_exit );

  return th;
}

void
thread_init ( thread_func f_start, void *f_exit );

void
thread_clean ( void );

void
thread_switch_extended ( struct thread *current, struct thread *new_th );

/* defined in thread_swap.S */

extern void
thread_set ( struct thread *th );

extern void
thread_switch ( struct thread *current, struct thread *new_th );

#endif
