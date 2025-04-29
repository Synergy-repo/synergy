
#include <assert.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <stdatomic.h>
#include <x86intrin.h>

#define WORKER_CORE 5

#define QUANTUM 5 /* in us */
#define TICKS_BY_US 2190

#define TSC_QUANTUM ( QUANTUM * TICKS_BY_US )

#define DISABLED 0
#define ENABLED 1
#define SENDED 2

static _Atomic int state = DISABLED;
static _Atomic uint64_t deadline;

static pid_t pid, tid;

void
timer_enable ( void )
{
  assert ( state == DISABLED );

  deadline = __rdtsc () + TSC_QUANTUM;
  state = ENABLED;
}

void
timer_received ( void )
{
  assert ( state == SENDED );
  state = DISABLED;
}

void
timer_init ( pid_t t_id )
{
  tid = t_id;
}

void
timer_main ( void )
{
  pid = getpid ();

  while ( 1 )
    {
      while ( state != ENABLED )
        ;

      uint64_t now = __rdtsc ();

      if ( now < deadline )
        continue;

      // int expected = DISABLED;
      // if ( !atomic_compare_exchange_strong ( &state, &expected, SENDED ) )
      //  continue;

      state = SENDED;

      // interrupt_send ( WORKER_CORE );
      syscall ( SYS_tgkill, pid, tid, SIGUSR1 );
    }
}
