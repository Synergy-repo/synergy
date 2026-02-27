
#ifndef IPI_H
#define IPI_H

typedef void ( *user_handler ) ( void );

/* trap bitmask. One bit position per CPU. bit x is CPU x */
struct ipi_cpumask
{
  /*TODO: limit of 64 cpus to x86-64 is good for now? */
  unsigned long bits;
};

#define ipi_cpumask_add( cpu, mask ) ( mask.bits |= ( 1UL << ( cpu ) ) )

#define ipi_cpumask_clear( mask ) ( mask.bits = 0UL )

#define ipi_cpumask_is_empty( mask ) ( 0 == mask.bits )

/* set high level user handler interruption in trap.c */
int
ipi_init ( user_handler handler );

/* init per worker */
int
ipi_init_worker ( void );

/* send interrupt to many cores present in 'cpumask' */
int
ipi_send_interrupt_many ( struct ipi_cpumask cpumask );

/* send interrupt to 'core' */
static inline int
ipi_send_interrupt ( int core )
{
  struct ipi_cpumask cpumask = { .bits = ( 1UL << ( unsigned ) core ) };
  return ipi_send_interrupt_many ( cpumask );
}

void
ipi_free ( void );

#endif
