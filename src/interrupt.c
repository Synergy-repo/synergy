
#define _GNU_SOURCE

#include <stdint.h>

#include "synergy_internal.h"
#include "debug.h"
#include "compiler.h"

#ifdef SIGNAL

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

static pid_t workers_tids[MAX_WORKERS];

/* bitmask where each bit enable represent worker ID to be interrupted */
static unsigned long workers_to_interrupt_mask;

static pid_t pid;

long
interrupt_send ( uint32_t wid )
{
  pid_t tid = workers_tids[wid];
  return syscall ( SYS_tgkill, pid, tid, SIGUSR1 );
}

bool
interrupt_has_worker_to_interrupt ( void )
{
  return ( workers_to_interrupt_mask != 0UL );
}

void
interrupt_many_workers ( void )
{
  unsigned long mask;
  for ( unsigned i = 0; i < MAX_WORKERS; i++ )
    {
      mask = ( 1UL << i );
      if ( workers_to_interrupt_mask & mask )
        {
          interrupt_send ( i );
        }
    }
}

void
interrupt_add_worker ( uint32_t wid )
{
  workers_to_interrupt_mask |= ( 1 << wid );
}

void
interrupt_clear_workers ( void )
{
  workers_to_interrupt_mask = 0UL;
}

void
interrupt_register_worker ( uint32_t wid )
{
  workers_tids[wid] = gettid ();
}

void
interrupt_init ( void ( *handler ) ( int ) )
{
  const struct sigaction sa = { .sa_handler = handler,
                                .sa_flags = SA_RESTART | SA_NODEFER };

  pid = getpid ();

  if ( -1 == sigaction ( SIGUSR1, &sa, NULL ) )
    FATAL ( "%s\n", "Error interrupt_init" );
}

#elif CONCORD

typedef void ( *user_handler ) ( void );

static int *workers_concord_flag[MAX_WORKERS];
static user_handler uhandler;

/* need be global because application should check this variable */
__thread int concord_preempt_now __aligned ( CACHE_LINE_SIZE ) = 0;

/* clang-format off */;
void interrupt_add_worker ( uint16_t __notused wid ) { assert(0); }
void interrupt_clear_workers ( void ) {assert(0); }
bool interrupt_has_worker_to_interrupt ( void ) { assert(0); return 0;}
void interrupt_many_workers ( void ) { assert(0); }
/* clang-format on */

void
concord_func ( void )
{
  concord_preempt_now = 0;
  uhandler ();
}

long
interrupt_send ( uint32_t wid )
{
  int *worker_interrupt = workers_concord_flag[wid];
  *worker_interrupt = 1;
  return 1;
}

void
interrupt_register_worker ( uint32_t wid )
{
  workers_concord_flag[wid] = &concord_preempt_now;
}

void
interrupt_init ( user_handler handler )
{
  if ( !handler )
    FATAL ( "%s\n", "Interrupt handler is NULL\n" );

  uhandler = handler;
}

#elif KMOD_IPI

#include <rte_lcore.h>
#include "../kmod/ipi.h"

static int workers_core[MAX_WORKERS];
static struct ipi_cpumask mask;

long
interrupt_send ( uint32_t wid )
{
  int core = workers_core[wid];
  return ipi_send_interrupt ( core );
}

void
interrupt_add_worker ( uint32_t wid )
{
  ipi_cpumask_add ( workers_core[wid], mask );
}

void
interrupt_clear_workers ( void )
{
  ipi_cpumask_clear ( mask );
}

bool
interrupt_has_worker_to_interrupt ( void )
{
  return !ipi_cpumask_is_empty ( mask );
}

void
interrupt_many_workers ( void )
{
  ipi_send_interrupt_many ( mask );
}

void
interrupt_register_worker ( uint32_t wid )
{
  workers_core[wid] = rte_lcore_id ();
  ipi_init_worker ();
}

void
interrupt_init ( user_handler handler )
{
  if ( ipi_init ( handler ) )
    FATAL ( "%s\n", "Error to init kmod_ipi\n" );
}

#elif UINTR

#include "../kmod/uintr.h"

static int workers_uitt_idx[MAX_WORKERS];

/* clang-format off */;
void interrupt_add_worker ( uint16_t __notused wid ) { assert(0); }
void interrupt_clear_workers ( void ) {assert(0); }
bool interrupt_has_worker_to_interrupt ( void ) { assert(0); return 0;}
void interrupt_many_workers ( void ) { assert(0); }
/* clang-format on */

long
interrupt_send ( uint32_t wid )
{
  int idx = workers_uitt_idx[wid];
  uintr_senduipi ( idx );
  return 1;
}

void
interrupt_register_worker ( uint32_t wid )
{
  int idx = uintr_init_worker ();
  if ( idx < 0 )
    FATAL ( "Error %d to init worker %u\n", idx, wid );

  workers_uitt_idx[wid] = idx;
}

void
interrupt_free_worker ( uint32_t wid )
{
  int idx = workers_uitt_idx[wid];
  uintr_free_worker ( idx );
}

void
interrupt_init ( user_handler handler )
{
  if ( uintr_init ( handler ) )
    FATAL ( "%s\n", "Error to init kmod_uintr\n" );

  /* ensure that timer core call this function and not change of core */
  uintr_init_sender ();
}

#else
#error("Interrupt method not defined");
#endif
