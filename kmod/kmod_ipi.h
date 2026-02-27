
#ifndef KMOD_IPI_H
#define KMOD_IPI_H

#include "ipi.h"

#define KMOD_IPI_NAME "kmod_ipi"
#define KMOD_IPI_PATH "/dev/kmod_ipi"

#define KMOD_IPI_MAJOR_NUM 280

struct req_ipi
{
  struct ipi_cpumask mask; /* send IPI to many cores */
};

// IOCTLs
#define KMOD_IPI_SEND _IOW ( KMOD_IPI_MAJOR_NUM, 0, struct req_ipi * )
#define KMOD_INIT _IOW ( KMOD_IPI_MAJOR_NUM, 1, unsigned long * )
#define KMOD_INIT_WORKER _IO ( KMOD_IPI_MAJOR_NUM, 2 )

#endif
