
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include <rte_flow.h>

#include "debug.h"

/* set specific source port to specific queue */
static void
flow_insert ( uint16_t port_id, uint16_t rxq, uint16_t udp_src_port )
{
  struct rte_flow_item_udp udp = { .hdr = {.src_port = ntohs ( udp_src_port ), .dst_port = 0} };
  struct rte_flow_item_udp udp_mask = { .hdr.src_port = 0xffff };

  struct rte_flow_item pattern[] = {
    { .type = RTE_FLOW_ITEM_TYPE_ETH },  /* pass all eth packets */
    { .type = RTE_FLOW_ITEM_TYPE_IPV4 }, /* pass all ipv4 packets */

    /* match source port udp */
    { .type = RTE_FLOW_ITEM_TYPE_UDP, .spec = &udp, .mask = &udp_mask },
    { .type = RTE_FLOW_ITEM_TYPE_END } /* end pattern match */
  };

  /* queue to send packet */
  struct rte_flow_action_queue queue = { .index = rxq };

  /* create the queue action */
  struct rte_flow_action action[] = { { .type = RTE_FLOW_ACTION_TYPE_QUEUE,
                                        .conf = &queue },
                                      { .type = RTE_FLOW_ACTION_TYPE_END } };

  /* validate and create the flow rule */
  struct rte_flow_attr attr = { .ingress = 1 }; /* match only ingress traffic */
  struct rte_flow_error error;

  if ( rte_flow_validate ( port_id, &attr, pattern, action, &error ) )
    FATAL ( "Error rte_flow_validate: %s\n", error.message );

  if ( !rte_flow_create ( port_id, &attr, pattern, action, &error ) )
    FATAL ( "Error rte_flow_create: %s\n", error.message );
}

// TODO: specific config
void
flow_init ( uint16_t port_id, uint16_t num_queues )
{
  uint16_t port, queue;

  INFO("%s\n", "FLOW DIRECTOR: ENABLE");

  for ( uint16_t i = 0; i < 518; i++ )
    {
      port = 1024 + i;
      queue = i % num_queues;

      DEBUG ( "Setting src port %u to queue %u\n", port, queue );
      flow_insert ( port_id, queue, port );
    }
}
