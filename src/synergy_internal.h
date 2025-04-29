
#ifndef synergy_INTERNAL_H
#define synergy_INTERNAL_H

/* bitmask limit: 64 */
#define MAX_WORKERS ( sizeof ( uint64_t ) * 8 )

/* soft queue size */
#define QUEUE_SIZE 128

/* DPDK hardware queue and burst */
#define QUEUE_SIZE_RX 128U
#define QUEUE_SIZE_TX 128U
#define BURST_SIZE 8

/* DPDK mempool size */
#define MPOOL_NR_ELEMENTS ( 16 * 1024 - 1 )
#define MPOOL_CACHE_SIZE 512U

/* max requests to stolen */
#define MAX_STOLEN 32

/* long requests queue */
#define WAIT_QUEUE_SIZE ( 16 * 1024 )

/* user threads pool */
#define THREAD_POOL_SIZE ( 32 * 1024 )
#define THREAD_POOL_CACHE_SIZE 512U
#define THREAD_LAZY_FREE_SIZE ( THREAD_POOL_CACHE_SIZE / 2 )

/* user threads stack size  in bytes */
#define STACK_SIZE ( 64 * 1024 )

#endif /* synergy_INTERNAL_H */
