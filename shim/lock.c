
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "shim_common.h"

__thread unsigned in_critical;

typedef struct my_mutex
{
  volatile int _m_lock;
} mutex_t;

#define pthread_mutex_t mutex_t

static inline int
a_cas (volatile int *p, int t, int s)
{
  __asm__ __volatile__("lock ; cmpxchg %3, %1"
                       : "=a"(t), "=m"(*p)
                       : "a"(t), "r"(s)
                       : "memory");
  return t;
}

static int
try_lock (pthread_mutex_t *m)
{
  return a_cas (&m->_m_lock, 0, 1);
  // static int ( *real_pthread_mutex_trylock ) ( pthread_mutex_t * mutex );
  // INIT_SYMBOL ( pthread_mutex_trylock );

  // return real_pthread_mutex_trylock ( mutex );
}

int
pthread_mutex_lock (pthread_mutex_t *m)
{
  NOTSELF (pthread_mutex_lock, m);

  int ret;
  while (1)
    {
      shim_disable_interrupt ();

      ret = try_lock (m);
      if (!ret)
        break;

      /* yield if not in crictal section */
      shim_force_yield ();
    }

  return ret;
}

int
pthread_mutex_trylock (pthread_mutex_t *m)
{
  NOTSELF (pthread_mutex_trylock, m);

  shim_disable_interrupt ();

  int ret = try_lock (m);

  /* lock not taken */
  if (ret)
    shim_enable_interrupt ();

  return ret;
}

int
pthread_mutex_unlock (pthread_mutex_t *m)
{
  NOTSELF (pthread_mutex_unlock, m)

  int ret = a_cas (&m->_m_lock, 1, 0);

  /* lock released */
  if (ret)
    shim_enable_interrupt ();

  return !ret;
}
