
#ifndef KMOD_UINTR_H
#define KMOD_UINTR_H

#include <linux/ioctl.h>

#define X86_UINTR_SUPPORT_BIT 5

static inline void
kmod_uintr_cpuid ( unsigned int *eax,
                   unsigned int *ebx,
                   unsigned int *ecx,
                   unsigned int *edx )
{
  /* ecx is often an input as well as an output. */
  asm volatile ( "cpuid"
                 : "=a"( *eax ), "=b"( *ebx ), "=c"( *ecx ), "=d"( *edx )
                 : "0"( *eax ), "2"( *ecx )
                 : "memory" );
}

/* return 0 if processor support uintr feature */
static inline int
uintr_processor_support ( void )
{
  unsigned a, b, c, d;
  a = 0x7;
  c = 0;
  kmod_uintr_cpuid ( &a, &b, &c, &d );

  return !( ( unsigned long ) d & ( 1UL << X86_UINTR_SUPPORT_BIT ) );
}

#define KMOD_UINTR_NAME "kmod_uintr"
#define KMOD_UINTR_PATH "/dev/kmod_uintr"

#define KMOD_UINTR_MAJOR_NUM 281

// IOCTLs
#define KMOD_UINTR_INIT_RECEIVER \
  _IOWR ( KMOD_UINTR_MAJOR_NUM, 0, unsigned long )
#define KMOD_UINTR_INIT_SENDER _IO ( KMOD_UINTR_MAJOR_NUM, 1 )
#define KMOD_UINTR_FREE _IOR ( KMOD_UINTR_MAJOR_NUM, 2, unsigned long )

#endif
