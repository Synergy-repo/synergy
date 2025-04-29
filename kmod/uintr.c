
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "kmod_uintr.h"
#include "uintr.h"

static user_handler uintr_u_handler;

static int fd = -1;

/* low level trap function in uintr_trap.S */
extern void
_uintr_trap_entry ( void );

/* this is called by _uintr_trap_entry defined in uintr_trap.S */
void
_uintr_handler ( void )
{
  /* call user defined handler */
  uintr_u_handler ();
}

/* enable UIF */
static inline void
_uintr_stui ( void )
{
  asm volatile ( "stui" ::: "memory" );
}

/* disable UIF */
static inline void
_uintr_clui ( void )
{
  asm volatile ( "clui" ::: "memory" );
}

int
uintr_init ( user_handler h )
{

  if ( uintr_processor_support () )
    return -1;

  if ( !h )
    return -1;

  fd = open ( KMOD_UINTR_PATH, O_RDWR );
  if ( fd < 0 )
    return -1;

  uintr_u_handler = h;

  return 0;
}

int
uintr_init_sender ( void )
{
  return ioctl ( fd, KMOD_UINTR_INIT_SENDER );
}

/* return index of UITT  that can be used by SENDUIPI */
int
uintr_init_worker ( void )
{
  int ret;
  uintptr_t data = ( uintptr_t ) _uintr_trap_entry;
  ret = ioctl ( fd, KMOD_UINTR_INIT_RECEIVER, &data );

  if ( !ret )
    {
      _uintr_stui ();
      ret = data;
    }

  return ret;
}

int
uintr_free_worker ( int idx )
{
  _uintr_clui ();
  return ioctl ( fd, KMOD_UINTR_FREE, idx );
}

void
uintr_free ( void )
{
  close ( fd );
}
