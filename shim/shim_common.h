
#pragma once

#include <assert.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/compiler.h"
#include "../src/globals.h"

extern void timer_disable (uint16_t);
extern void yield_to_main (void);

extern __thread unsigned in_critical;

static inline bool
shim_active (void)
{
  return allocator_on;
}

static inline void
shim_yield (void)
{
  yield_to_main ();
}

static inline void
shim_disable_interrupt (void)
{
  if (shim_active () && CURRENT_IS_LONG ())
    {
      CURRENT_DISABLE_RESCHEDULING ();
      in_critical++;
    }
}

static inline void
shim_enable_interrupt (void)
{
  if (shim_active () && CURRENT_IS_LONG ())
    {
      assert (in_critical);
      in_critical--;
      if (!in_critical)
        {
          if (CURRENT_SHOULD_YIELD ())
            {
              shim_yield ();
            }
          CURRENT_ENABLE_RESCHEDULING ();
        }
    }
}

static inline void
shim_force_yield (void)
{
  if (shim_active () && CURRENT_IS_LONG ())
    {
      assert (in_critical);
      in_critical--;
      if (!in_critical)
        {
          timer_disable (worker_id);
          shim_yield ();
        }
    }
}

#define INIT_SYMBOL(sym)                                                      \
  do                                                                          \
    {                                                                         \
      if (!real_##sym)                                                        \
        {                                                                     \
          real_##sym = (typeof (real_##sym))dlsym (RTLD_NEXT, #sym);          \
          if (!real_##sym)                                                    \
            {                                                                 \
              exit (EXIT_FAILURE);                                            \
            }                                                                 \
        }                                                                     \
    }                                                                         \
  while (0)

#define NOTSELF(name, ...)                                                    \
  if (unlikely (!shim_active ()))                                             \
    {                                                                         \
      static typeof (name) *fn;                                               \
      if (!fn)                                                                \
        {                                                                     \
          fn = dlsym (RTLD_NEXT, #name);                                      \
          assert (fn);                                                        \
        }                                                                     \
      return fn (__VA_ARGS__);                                                \
    }
