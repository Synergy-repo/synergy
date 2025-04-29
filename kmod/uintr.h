
#ifndef UINTR_H
#define UINTR_H

typedef void ( *user_handler ) ( void );

int
uintr_init ( user_handler h );

/* return index to usage with SENDUIPI */
int
uintr_init_worker ( void );

int
uintr_init_sender ( void );

int
uintr_free_worker ( int idx );

void
uintr_free ( void );

/* force operando of 64 bits */
static inline void
uintr_senduipi ( uint64_t uitt_idx )
{
  asm volatile ( "senduipi %0" ::"r"( uitt_idx ) : "memory" );
}

#endif
