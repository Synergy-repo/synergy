
#include <stdlib.h>

#include "shim_common.h"

#define HOOK3(fnname, retType, argType1, argType2, argType3)                  \
  retType fnname (argType1 __a1, argType2 __a2, argType3 __a3)                \
  {                                                                           \
    static retType (*real_##fnname) (argType1, argType2, argType3);           \
    if (unlikely (!real_##fnname))                                            \
      {                                                                       \
        real_##fnname = dlsym (RTLD_NEXT, #fnname);                           \
      }                                                                       \
    shim_disable_interrupt ();                                                \
    retType __t = real_##fnname (__a1, __a2, __a3);                           \
    shim_enable_interrupt ();                                                 \
    return __t;                                                               \
  }

#define HOOK2(fnname, retType, argType1, argType2)                            \
  retType fnname (argType1 __a1, argType2 __a2)                               \
  {                                                                           \
    static retType (*real_##fnname) (argType1, argType2);                     \
    if (unlikely (!real_##fnname))                                            \
      {                                                                       \
        real_##fnname = dlsym (RTLD_NEXT, #fnname);                           \
      }                                                                       \
    shim_disable_interrupt ();                                                \
    retType __t = real_##fnname (__a1, __a2);                                 \
    shim_enable_interrupt ();                                                 \
    return __t;                                                               \
  }

#define HOOK1(fnname, retType, argType1)                                      \
  retType fnname (argType1 __a1)                                              \
  {                                                                           \
    static retType (*real_##fnname) (argType1);                               \
    if (unlikely (!real_##fnname))                                            \
      {                                                                       \
        real_##fnname = dlsym (RTLD_NEXT, #fnname);                           \
      }                                                                       \
    shim_disable_interrupt ();                                                \
    retType __t = real_##fnname (__a1);                                       \
    shim_enable_interrupt ();                                                 \
    return __t;                                                               \
  }

#define HOOK1_NORET(fnname, argType1)                                         \
  void fnname (argType1 __a1)                                                 \
  {                                                                           \
    static void (*real_##fnname) (argType1);                                  \
    if (unlikely (!real_##fnname))                                            \
      {                                                                       \
        real_##fnname = dlsym (RTLD_NEXT, #fnname);                           \
      }                                                                       \
    shim_disable_interrupt ();                                                \
    real_##fnname (__a1);                                                     \
    shim_enable_interrupt ();                                                 \
  }

HOOK1 (malloc, void *, size_t);
HOOK1_NORET (free, void *);
HOOK2 (realloc, void *, void *, size_t);
HOOK1_NORET (cfree, void *);
HOOK2 (memalign, void *, size_t, size_t);
HOOK2 (aligned_alloc, void *, size_t, size_t);
HOOK1 (valloc, void *, size_t);
HOOK1 (pvalloc, void *, size_t);
HOOK3 (posix_memalign, int, void **, size_t, size_t);

HOOK1_NORET (__libc_free, void *);
HOOK2 (__libc_realloc, void *, void *, size_t);
HOOK2 (__libc_calloc, void *, size_t, size_t);
HOOK1_NORET (__libc_cfree, void *);
HOOK2 (__libc_memalign, void *, size_t, size_t);
HOOK1 (__libc_valloc, void *, size_t);
HOOK1 (__libc_pvalloc, void *, size_t);
HOOK3 (__posix_memalign, int, void **, size_t, size_t);
