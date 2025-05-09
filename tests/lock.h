
#ifndef LOCK_H
#define LOCK_H

#include <stdbool.h>

#define SPINLOCK_INIT \
  {                   \
    .locked = 0       \
  }

typedef struct
{
  volatile int locked;
} spinlock_t;

static inline void
cpu_relax ( void )
{
  __asm__ volatile( "pause" );
}

/**
 * spin_lock_init - prepares a spin lock for use
 * @l: the spin lock
 */
static inline void
spin_lock_init ( spinlock_t *l )
{
  l->locked = 0;
}

/**
 * spin_lock - takes a spin lock
 * @l: the spin lock
 */
static inline void
spin_lock ( spinlock_t *l )
{
  while ( __sync_lock_test_and_set ( &l->locked, 1 ) )
    {
      while ( l->locked )
        cpu_relax ();
    }
}

/**
 * spin_try_lock- takes a spin lock, but only if it is available
 * @l: the spin lock
 *
 * Returns 1 if successful, otherwise 0
 */
static inline bool
spin_try_lock ( spinlock_t *l )
{
  if ( !__sync_lock_test_and_set ( &l->locked, 1 ) )
    return true;
  return false;
}

/**
 * spin_unlock - releases a spin lock
 * @l: the spin lock
 */
static inline void
spin_unlock ( spinlock_t *l )
{
  __sync_lock_release ( &l->locked );
}

#endif
