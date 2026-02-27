
#ifndef synergy_NETIO_H
#define synergy_NETIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <rte_ring_core.h>
#include <rte_ring_elem.h>

#include "synergy_internal.h"
#include "thread.h"
#include "synergy.h"

void
netio_init ( uint16_t p_id, uint16_t rx_queues, uint16_t tx_queues );

void
netio_init_per_worker ( uint32_t w_id );

_Atomic uint32_t *
netio_busy_workers ( void );

struct rte_mbuf *
netio_recv ( void );

struct rte_mbuf *
netio_workstealing ( void );

struct thread *
netio_timer_recv ( void );

bool
netio_worker_has_new_jobs ( uint32_t wid );

int
netio_parse_pkt ( struct rte_mbuf *pkt,
                  void **payload,
                  uint16_t *payload_size );

uint16_t
netio_send ( void *buff, uint16_t buff_len, struct sock s );

unsigned
netio_worker_queue_count ( uint32_t wid );

/* return 0 if success */
int
netio_worker_enqueue ( void *obj );

void
netio_clean ( void );

#endif  // synergy_NETIO_H
