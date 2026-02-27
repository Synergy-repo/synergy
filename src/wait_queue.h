
#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

bool
wait_queue_check_congested ( uint64_t now );

void
wait_queue_enqueue ( void *obj );

void
wait_queue_timer_enqueue ( void *obj );

void *
wait_queue_dequeue ( void );

unsigned
wait_queue_count ( void );

int
wait_queue_empty ( void );

void
wait_queue_init ( void );

void
wait_queue_clean ( void );

#endif  // WAIT_QUEUE_H
