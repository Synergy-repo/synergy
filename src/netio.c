
/* network management */

#include <assert.h>
#include <stdint.h>

#include <rte_ethdev.h>
#include <rte_spinlock.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf_core.h>
#include <rte_udp.h>

#include "synergy_internal.h"
#include "compiler.h"
#include "debug.h"
#include "dpdk.h"
#include "globals.h"  // XXX: debug
#include "interrupt.h"
#include "netio.h"
#include "stats.h"
#include "thread.h"
#include "feedback.h"
#include "wait_queue.h"

static uint16_t port_id;
static struct rte_ether_addr my_ether;

static __thread uint16_t tx_hwq;

#ifdef ENABLE_CFCFS
static uint16_t hwq;
rte_spinlock_t hwq_lock = RTE_SPINLOCK_INITIALIZER;
static struct rte_ring *rxq; /* main queue */
#else

/* worker queues */
static struct rte_ring *rxqs[MAX_WORKERS];

static __thread uint16_t hwq;         /* hardware queue of this core */
static __thread struct rte_ring *rxq; /* soft queue of this core */

/* Array with pointer to remote workers queues with valid size of total worker
 * - 1. This array is NULL terminated.
 * This not contain queue of current worker. This is used for work stealing. */
static __thread struct rte_ring *remote_queues[MAX_WORKERS];

#endif

/* only timer core update this list with workers that are processing long
 * requests. Worker when doing work stealing first try this list. */
static _Atomic uint32_t busy_workers_id[MAX_WORKERS] __aligned (
        CACHE_LINE_SIZE ) = { [0 ... MAX_WORKERS - 1] = -1 };

_Atomic uint32_t *
netio_busy_workers ( void )
{
  return busy_workers_id;
}

#define SWAP( a, b )           \
  ( {                          \
    typeof ( a ) _tmp = ( a ); \
    ( a ) = ( b );             \
    ( b ) = _tmp;              \
  } )

#define MAKE_IP_ADDR( a, b, c, d )                        \
  ( ( ( uint32_t ) a << 24 ) | ( ( uint32_t ) b << 16 ) | \
    ( ( uint32_t ) c << 8 ) | ( uint32_t ) d )

#define MIN_PKT_SIZE                                     \
  ( RTE_ETHER_HDR_LEN + sizeof ( struct rte_ipv4_hdr ) + \
    sizeof ( struct rte_udp_hdr ) )

//#define NET_DEBUG
int
netio_parse_pkt ( struct rte_mbuf *pkt, void **payload, uint16_t *payload_size )
{
  rte_prefetch0 ( rte_pktmbuf_mtod ( pkt, void * ) );

#ifdef NET_DEBUG
  struct rte_ether_hdr *eth;
  eth = rte_pktmbuf_mtod ( pkt, struct rte_ether_hdr * );
  if ( !rte_is_same_ether_addr ( &eth->dst_addr, &my_ether ) )
    {
      DEBUG ( "%s\n", "Not for me" );
      DEBUG ( "DST MAC: %02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
              ":%02" PRIx8 ":%02" PRIx8 "\n",
              RTE_ETHER_ADDR_BYTES ( &eth->dst_addr ) );
      DEBUG ( "My MAC: %02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
              ":%02" PRIx8 ":%02" PRIx8 "\n",
              RTE_ETHER_ADDR_BYTES ( &eth->src_addr ) );
      rte_pktmbuf_free ( pkt );
      return -1;
    }

  if ( eth->ether_type != rte_cpu_to_be_16 ( RTE_ETHER_TYPE_IPV4 ) )
    {
      DEBUG ( "%s\n", "Not IPv4" );
      rte_pktmbuf_free ( pkt );
      return -1;
    }

  uint32_t offset = sizeof ( struct rte_ether_hdr );
  struct rte_ipv4_hdr *ip;
  ip = rte_pktmbuf_mtod_offset ( pkt, struct rte_ipv4_hdr *, offset );

  if ( ( ip->next_proto_id != IPPROTO_UDP ) )
    {
      DEBUG ( "%s\n", "Not UDP" );
      rte_pktmbuf_free ( pkt );
      return -1;
    }

#endif  // NET_DEBUG

  struct rte_udp_hdr *udp;
  udp = rte_pktmbuf_mtod_offset ( pkt,
                                  struct rte_udp_hdr *,
                                  RTE_ETHER_HDR_LEN +
                                          sizeof ( struct rte_ipv4_hdr ) );

  *payload_size = rte_be_to_cpu_16 ( udp->dgram_len ) - sizeof ( *udp );
  *payload = rte_pktmbuf_mtod_offset ( pkt, void *, MIN_PKT_SIZE );

  return 0;
}

bool
netio_worker_has_new_jobs ( uint32_t __notused wid )
{
#ifdef ENABLE_CFCFS
  /* worker id is equal worker queue id */
  return !rte_ring_empty ( rxq ) ||
         ( rte_eth_rx_queue_count ( port_id, 0 ) > 0 );
#else
  /* worker id is equal worker queue id */
  return !rte_ring_empty ( rxqs[wid] ) ||
         ( rte_eth_rx_queue_count ( port_id, wid ) > 0 );
#endif
}

unsigned
netio_worker_queue_count ( uint32_t __notused wid )
{
  unsigned ret;
#ifdef ENABLE_CFCFS
  ret = rte_ring_count ( rxq ) + rte_eth_rx_queue_count ( port_id, 0 );
#else
  ret = rte_ring_count ( rxqs[wid] ) + rte_eth_rx_queue_count ( port_id, wid );
#endif
  return ret;
}

// return true if has work on queue
static bool
check_hard_queue ( struct rte_ring *myq, uint16_t hardq, struct rte_mbuf **pkt )
{
  struct rte_mbuf *pkts[BURST_SIZE] __aligned ( CACHE_LINE_SIZE );
  unsigned free;
  uint16_t nb_rx;

  nb_rx = rte_eth_rx_burst ( port_id, hardq, pkts, BURST_SIZE );
  if ( !nb_rx )
    return false;

  STAT ( rx_count, nb_rx );

#ifdef ENABLE_WAIT_QUEUE
  /* ensure that this work process some request and avoid be stolen all requets.
   */
  *pkt = pkts[0];
  nb_rx--;
  rte_ring_sp_enqueue_burst ( myq, ( void ** ) &pkts[1], nb_rx, &free );
#else
  rte_ring_sp_enqueue_burst ( myq, ( void ** ) pkts, nb_rx, &free );
#endif

  return true;
}

/*
 * div_up - divides two numbers, rounding up to an integer
 * @x: the dividend
 */
#define div_up( x, d ) ( ( ( ( x ) + ( d ) -1 ) ) / ( d ) )

#define min( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )

#ifdef ENABLE_WORKSTEALING

// return amount work stolen
static inline uint32_t
try_steal ( struct rte_ring *my,
            struct rte_ring *remote,
            unsigned min_batch,
            struct rte_mbuf **pkt )
{
  void *objs[MAX_STOLEN] __aligned ( CACHE_LINE_SIZE );
  uint32_t size;

  size = rte_ring_count ( remote );

  if ( size < min_batch )
    return 0;

  size = div_up ( size, 2 );
  size = min ( size, MAX_STOLEN );
  size = rte_ring_mc_dequeue_burst ( remote, objs, size, NULL );

  if ( !size )
    return 0;

  /* get one thread */
  *pkt = objs[0];

  /* enqueue remainder */
  size = rte_ring_sp_enqueue_burst ( my, &objs[1], size - 1, NULL );

  return size + 1;
}

static inline struct rte_mbuf *
check_busy_workers ( struct rte_ring *my_queue, unsigned min_batch )
{
  struct rte_mbuf *pkt = NULL;
  _Atomic uint32_t *busy_wid = busy_workers_id;

  uint32_t wid = *busy_wid++;
  while ( wid != ( uint32_t ) -1 )
    {
      uint32_t steal = try_steal ( my_queue, rxqs[wid], min_batch, &pkt );
      if ( steal )
        {
          STAT ( stealing, steal );
          break;
        }

      wid = *busy_wid++;
    }

  return pkt;
}

static inline struct rte_mbuf *
check_all_worker ( struct rte_ring *my_queue, unsigned min_batch )
{
  struct rte_mbuf *pkt = NULL;
  struct rte_ring **remote;

  for ( remote = remote_queues; *remote != NULL; remote++ )
    {

      uint32_t steal = try_steal ( my_queue, *remote, min_batch, &pkt );
      if ( steal )
        {
          STAT ( stealing, steal );
          break;
        }
    }

  return pkt;
}

static struct rte_mbuf *
check_remote_work ( struct rte_ring *my_queue )
{
  struct rte_mbuf *pkt;

#ifdef ENABLE_WORKSTEALING_BW
  pkt = check_busy_workers ( my_queue, 1 );
  if ( pkt )
    return pkt;
#endif

  pkt = check_all_worker ( my_queue, 2 );

  return pkt;
}

struct rte_mbuf *
netio_workstealing ( void )
{
  return check_remote_work ( rxq );
}

#endif  // WORKSTEALING

int
netio_worker_enqueue ( void *obj )
{
#ifdef ENABLE_CFCFS
  return rte_ring_mp_enqueue ( rxq, obj );
#else

#ifndef ENABLE_WAIT_QUEUE
  /* If wait queue is not enable, us always check hardwaue queue and after
   * enqueue the 'obj' on tail of queue.
   * This avoid that request get stuck on hardware. */
  check_hard_queue ( rxq, hwq, NULL );
#endif

  return rte_ring_sp_enqueue ( rxq, obj );

#endif
}

static inline int
soft_queue_dequeue ( struct rte_ring *q, void **obj )
{
#if defined( ENABLE_WORKSTEALING ) || defined( ENABLE_CFCFS )
  return rte_ring_mc_dequeue ( q, obj );
#else
  return rte_ring_sc_dequeue ( q, obj );
#endif
}

struct rte_mbuf *
netio_recv ( void )
{
  struct rte_mbuf *pkt = NULL;

  /* when wait queue is enabled (its is synergy) and we only check hard queue if
   * soft queue is empty. This improve batching when checking hard queue and
   * avoid many hard queue checking, that can be expensive.*/
  if ( !soft_queue_dequeue ( rxq, ( void ** ) &pkt ) )
    goto done;

#ifdef ENABLE_CFCFS
  if ( rte_spinlock_trylock ( &hwq_lock ) )
    {
#endif

      check_hard_queue ( rxq, hwq, &pkt );

#ifdef ENABLE_CFCFS
      rte_spinlock_unlock ( &hwq_lock );
    }
#endif

done:
  return pkt;
}

uint16_t
netio_send ( void *buff, uint16_t len, struct sock s )
{
  synergy_feedback_finished_request ();

  ( void ) buff;
  struct rte_mbuf *pkt = s.pkt;

  // offloading cksum to hardware
  pkt->ol_flags |= ( RTE_MBUF_F_TX_IPV4 | RTE_MBUF_F_TX_IP_CKSUM );

  struct rte_ether_hdr *eth_hdr;
  eth_hdr = rte_pktmbuf_mtod ( pkt, struct rte_ether_hdr * );
  eth_hdr->dst_addr = eth_hdr->src_addr;
  eth_hdr->src_addr = my_ether;

  uint32_t offset = sizeof ( struct rte_ether_hdr );
  struct rte_ipv4_hdr *ip_hdr;
  ip_hdr = rte_pktmbuf_mtod_offset ( pkt, struct rte_ipv4_hdr *, offset );
  ip_hdr->ihl = 5;
  ip_hdr->version = 4;
  ip_hdr->total_length =
          rte_cpu_to_be_16 ( sizeof ( struct rte_ipv4_hdr ) +
                             sizeof ( struct rte_udp_hdr ) + len );

  ip_hdr->packet_id = 0;
  ip_hdr->fragment_offset = 0;

  ip_hdr->time_to_live = 64;
  ip_hdr->next_proto_id = IPPROTO_UDP;
  ip_hdr->hdr_checksum = 0;
  // ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

  SWAP ( ip_hdr->src_addr, ip_hdr->dst_addr );

  offset += sizeof ( struct rte_ipv4_hdr );
  struct rte_udp_hdr *udp_hdr;
  udp_hdr = rte_pktmbuf_mtod_offset ( pkt, struct rte_udp_hdr *, offset );
  SWAP ( udp_hdr->src_port, udp_hdr->dst_port );

  udp_hdr->dgram_cksum = 0;

  // We not modify payload of client request, only echo this
  // offset += sizeof ( struct rte_udp_hdr );
  // void *payload = rte_pktmbuf_mtod_offset ( pkt, void *, offset );
  // rte_memcpy ( payload, buff, len );

  pkt->l2_len = sizeof ( struct rte_ether_hdr );
  pkt->l3_len = sizeof ( struct rte_ipv4_hdr );
  // pkt->l4_len = sizeof ( struct rte_udp_hdr );

  // Total size packet
  pkt->pkt_len = pkt->data_len = MIN_PKT_SIZE + len;

  // XXX: debug
  //( ( uint64_t * ) buff )[7] = rte_get_tsc_cycles ();
  uint16_t ret = rte_eth_tx_burst ( port_id, tx_hwq, &pkt, 1 );

  return ret;
}

void
netio_init ( uint16_t p_id, uint16_t rx_queues, uint16_t tx_queues )
{
  char name[16];
  port_id = p_id;
  dpdk_init ( port_id, rx_queues, tx_queues );

  rte_eth_macaddr_get ( port_id, &my_ether );

#ifdef ENABLE_CFCFS
  hwq = 0;
  rxq = rte_ring_create ( name, QUEUE_SIZE, rte_socket_id (), 0 );
  if ( !rxq )
    FATAL ( "Error to alloc main queue: %s\n", rte_strerror ( rte_errno ) );

#else

  uint16_t tot_soft_queues = rx_queues;

  /* alloc and init workers soft queues */
  for ( uint16_t i = 0; i < tot_soft_queues; i++ )
    {
      snprintf ( name, sizeof name, "rxq%u", i );

      rxqs[i] = rte_ring_create ( name, QUEUE_SIZE, rte_socket_id (), 0 );
      if ( !rxqs[i] )
        FATAL ( "Error to alloc queue to worker %u: %s\n",
                i,
                rte_strerror ( rte_errno ) );
    }
#endif
}

void
netio_clean ( void )
{
#ifdef ENABLE_CFCFS
  rte_ring_free ( rxq );
#else
  unsigned i = 0;
  while ( rxqs[i] && i < MAX_WORKERS )
    rte_ring_free ( rxqs[i++] );
#endif
}

void
netio_init_per_worker ( uint32_t __notused wid )
{
  tx_hwq = wid;

#ifndef ENABLE_CFCFS
  hwq = wid;
  rxq = rxqs[wid];

  uint16_t num_workers = tot_workers;

  uint16_t remote = 0;
  for ( uint16_t i = 1; i < num_workers; i++ )
    {
      uint16_t next = ( ( wid + i ) % num_workers );

      remote_queues[remote++] = rxqs[next];
    }
  remote_queues[num_workers] = NULL;
#endif
}
