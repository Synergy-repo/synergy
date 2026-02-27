
/* User level thread */

#include <cpuid.h>

#include <rte_mempool.h>
#include <rte_errno.h>

#include "thread.h"
#include "compiler.h"
#include "synergy_internal.h"
#include "debug.h"

struct rte_mempool *thread_pool;

thread_func fn_start;
void *dfl_fn_exit;

/* maximum size in bytes needed for xsave */
size_t xsave_max_size;
/* extended processor features to save */
size_t xsave_features;

static void
init_user_thread ( struct rte_mempool __notused *mp,
                   void __notused *opaque,
                   void *obj,
                   unsigned __notused obj_idx )
{
  struct thread *th = obj;
  *th->stack = CANARY;
}

static struct rte_mempool *
create_pool ( char *name, unsigned elt_size )
{
  return rte_mempool_create ( name,
                              THREAD_POOL_SIZE,
                              elt_size,
                              THREAD_POOL_CACHE_SIZE,
                              0,
                              NULL,
                              NULL,
                              init_user_thread,
                              NULL,
                              rte_socket_id (),
                              0 );
}

void
thread_init ( thread_func f_start, void *f_exit )
{
  fn_start = f_start;
  dfl_fn_exit = f_exit;

  thread_pool = create_pool ( "thread_pool", sizeof ( struct thread ) );
  if ( !thread_pool )
    FATAL ( "Error to alloc thread pool: %s\n", rte_strerror ( rte_errno ) );

  unsigned a, b, c, d;
  if ( !__get_cpuid_count ( 0xD, 0, &a, &b, &c, &d ) || a == 0 || c == 0 )
    FATAL ( "%s\n",
            "Error to read XSAVE features,"
            "maybe this processor not support this feature" );

  /* intel vol.1 13.2*/
  xsave_features = ( ( uint64_t ) d << 32 ) | a;
  if ( !( xsave_features & 1 ) )
    FATAL ( "%s\n", "Process not support XSAVE FEATURE" );

  xsave_max_size = c;

  DEBUG ( "XSAVE features: 0x%lX\n", xsave_features );
  DEBUG ( "XSAVE area size: %ld bytes\n", xsave_max_size );
}

void
thread_clean ( void )
{
  rte_mempool_free ( thread_pool );
}

static inline void
clear64xbuff ( void *ptr )
{
  __asm__ volatile( "xor %%rax, %%rax\n\t"  // Set RAX to 0
                    "mov $8, %%rcx\n\t"     // 8 QWORDs (8 x 8 bytes = 64 bytes)
                    "rep stosq\n\t"         // Write RCX QWORDs (0) to [RDI]
                    :
                    : "D"( ptr )  // RDI = ptr
                    : "rax", "rcx", "memory" );
}

void __nofp
thread_switch_extended ( struct thread *prev, struct thread *next )
{
#if defined KMOD_IPI || defined UINTR
  /* allocate xsave area and clear xsave header */
  unsigned char xsave_buff[xsave_max_size] __aligned ( 64 );

  clear64xbuff ( xsave_buff + 512 );

  /* get only in use extented states */
  uint64_t active_xstates = __builtin_ia32_xgetbv ( 1 );

  /* use init optimization */
  __builtin_ia32_xsavec64 ( xsave_buff, active_xstates );
#endif

  thread_switch ( prev, next );

#if defined KMOD_IPI || defined UINTR
  __builtin_ia32_xrstor64 ( xsave_buff, active_xstates );
#endif
}
