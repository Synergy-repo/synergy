
#include <rte_errno.h>
#include <rte_lcore.h>
#include <stdint.h>
#include <stdlib.h>

#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <sys/types.h>

#include "synergy_internal.h"
#include "debug.h"

static const struct rte_eth_conf conf = {
  .rxmode = { .mq_mode = RTE_ETH_MQ_RX_RSS },
  .rx_adv_conf = { .rss_conf = { .rss_hf = RTE_ETH_RSS_NONFRAG_IPV4_UDP } },
  .txmode = { .mq_mode = RTE_ETH_MQ_TX_NONE,
              .offloads = RTE_ETH_TX_OFFLOAD_IPV4_CKSUM |
                          RTE_ETH_TX_OFFLOAD_UDP_CKSUM },

};

static void
port_init ( uint16_t port_id,
            uint16_t rx_queues,
            uint16_t tx_queues,
            struct rte_mempool *mbuf_pool )
{
  int ret;
  ret = rte_eth_dev_configure ( port_id, rx_queues, tx_queues, &conf );
  if ( ret != 0 )
    ERROR ( "Error config port id %d\n", port_id );

  uint16_t nb_rx = QUEUE_SIZE_RX;
  uint16_t nb_tx = QUEUE_SIZE_TX;

  ret = rte_eth_dev_adjust_nb_rx_tx_desc ( port_id, &nb_rx, &nb_tx );
  if ( ret != 0 )
    rte_exit ( EXIT_FAILURE, "Error set queue size" );

  int numa_id = rte_eth_dev_socket_id ( port_id );
  if ( numa_id == -1 )
    rte_exit ( EXIT_FAILURE, "Error to get numa ID" );

  /* Allocate and set up QUEUE_COUNT RX/TX queue per Ethernet port. */

  struct rte_eth_dev_info dev_info;
  rte_eth_dev_info_get ( port_id, &dev_info );

  struct rte_eth_rxconf *rxconf = &dev_info.default_rxconf;
  rxconf->rx_free_thresh = 32;
  uint16_t q;
  for ( q = 0; q < rx_queues; q++ )
    {
      ret = rte_eth_rx_queue_setup (
              port_id, q, nb_rx, numa_id, rxconf, mbuf_pool );
      if ( ret != 0 )
        rte_exit ( EXIT_FAILURE, "Error allocate RX queue" );
    }

  /*https://doc.dpdk.org/guides-23.11/prog_guide/poll_mode_drv.html*/
  struct rte_eth_txconf *txconf = &dev_info.default_txconf;
  txconf->tx_thresh.wthresh = 0;
  txconf->tx_rs_thresh = 32;
  txconf->tx_free_thresh = 32;
  // txconf->offloads =
  //         RTE_ETH_TX_OFFLOAD_IPV4_CKSUM | RTE_ETH_TX_OFFLOAD_UDP_CKSUM;

  for ( q = 0; q < tx_queues; q++ )
    {
      ret = rte_eth_tx_queue_setup ( port_id, q, nb_tx, numa_id, txconf );
      if ( ret != 0 )
        rte_exit ( EXIT_FAILURE, "Error allocate TX queue\n" );
    }

  /* disable flow control */

  struct rte_eth_fc_conf fc_conf;
  ret = rte_eth_dev_flow_ctrl_get ( port_id, &fc_conf );
  if ( ret == 0 )
    {
      fc_conf.mode = RTE_ETH_FC_NONE;
      ret = rte_eth_dev_flow_ctrl_set ( port_id, &fc_conf );
      if ( ret == 0 )
        INFO ( "%s\n", "Flow control disabled" );
      else
        WARNING ( "%s\n", "Error to disable flow control" );
    }
  else
    WARNING ( "%s\n", "Error to get fow control config" );

  ret = rte_eth_dev_start ( port_id );
  if ( ret != 0 )
    rte_exit ( EXIT_FAILURE, "Error start port id %d\n", port_id );

  struct rte_ether_addr addr;
  ret = rte_eth_macaddr_get ( port_id, &addr );
  if ( ret != 0 )
    rte_exit ( EXIT_FAILURE,
               "Error to get mac address: %s\n",
               rte_strerror ( rte_errno ) );

  struct rte_eth_link link;
  ret = rte_eth_link_get ( port_id, &link );
  assert ( ret == 0 );

  /* Display the port infos. */
  INFO ( "Port speed: %uMbps duplex: %s "
         "MAC: %02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
         ":%02" PRIx8 "\n",
         link.link_speed,
         link.link_duplex == RTE_ETH_LINK_FULL_DUPLEX ? "Full" : "Half",
         RTE_ETHER_ADDR_BYTES ( &addr ) );
}

struct rte_mempool *
dpdk_init ( uint16_t port_id, uint16_t rx_queues, uint16_t tx_queues )
{
  struct rte_mempool *mbuf_pool;

  mbuf_pool = rte_pktmbuf_pool_create ( "mbuf_pool",
                                        MPOOL_NR_ELEMENTS,
                                        MPOOL_CACHE_SIZE,
                                        0,
                                        RTE_MBUF_DEFAULT_BUF_SIZE,
                                        rte_socket_id () );

  if ( !mbuf_pool )
    ERROR ( "Error alloc mbuf on socket %d: %s\n",
            rte_socket_id (),
            rte_strerror ( rte_errno ) );

  port_init ( port_id, rx_queues, tx_queues, mbuf_pool );

  return mbuf_pool;
}
