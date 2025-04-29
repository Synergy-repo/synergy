
#pragma once

#include <signal.h>
void
timer_enable ( void );

void
timer_received ( void );

void
timer_init ( pid_t t_id );

void
timer_main ( void );
