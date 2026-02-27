
#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>
#include <stdbool.h>

/* Interrupt a single worker */
long
interrupt_send ( uint32_t worker_id );

/* Add a worker to before interrupting */
void
interrupt_add_worker ( uint32_t worker_id );

/* Clear worker to interrupt */
void
interrupt_clear_workers ( void );

/* Interrupt many workers (added using interrupt_add_worker) */
void
interrupt_many_workers ( void );

bool
interrupt_has_worker_to_interrupt ( void );

void
interrupt_register_worker ( uint32_t worker_id );

#ifdef UINTR
void
interrupt_free_worker ( uint32_t idx );
#endif

void
interrupt_init ( void ( *handler ) ( int ) );

#endif
