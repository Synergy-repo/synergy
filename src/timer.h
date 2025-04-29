
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* timer states */
#define DISABLED 0
#define ACTIVE 1
#define SENDED 2
#define PAUSED 3

void
timer_enable ( uint32_t wid, uint64_t quantum_tsc );

void
timer_disable ( uint32_t wid );

void
timer_set_quantum_factor ( unsigned congested );

void
timer_init ( void );

void
timer_do_interrupts ( uint64_t now );

uint64_t
timer_get_avg_us_quantum ( void );

#endif
