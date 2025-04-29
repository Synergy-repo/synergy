
/* high level interface to kernel module kmod_ipi */

#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "ipi.h"
#include "kmod_ipi.h"

/* low level trap function in ipi_trap.S */
extern void
_ipi_entry ( void );

static int fd = -1;
static user_handler u_handler;

/* called from low level _ipi_entry in ipi_trap.S */
void
_ipi_handler ()
{
  /* call user level handler */
  u_handler ();
}

/* set high level user handler interruption in trap.c */
int
ipi_init ( user_handler h )
{
  fd = open ( KMOD_IPI_PATH, O_RDWR );
  if ( fd < 0 )
    return -1;

  u_handler = h;

  return ioctl ( fd, KMOD_INIT, _ipi_entry );
}

int
ipi_init_worker ( void )
{
  return ioctl ( fd, KMOD_INIT_WORKER );
}

int
ipi_send_interrupt_many ( struct ipi_cpumask cpumask )
{
  struct req_ipi req = { .mask = cpumask };
  return ioctl ( fd, KMOD_IPI_SEND, &req );
}

void
ipi_free ( void )
{
  close ( fd );
}
