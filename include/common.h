//
// Created by rewbycraft on 1/17/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_COMMON_H
#define SIXELPING_RECEIVER_DPDK_COMMON_H

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstdbool>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_launch.h>
#include <rte_malloc.h>
#include <rte_random.h>
#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_eventdev.h>
#include <rte_event_eth_rx_adapter.h>
#include <rte_event_eth_tx_adapter.h>
#include <rte_service.h>
#include <rte_service_component.h>
#include <rte_ether.h>
#include <rte_ip.h>

/* Macros for printing using RTE_LOG */
#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 256

extern "C" void nipktmfree(struct rte_mbuf *m);

#endif //SIXELPING_RECEIVER_DPDK_COMMON_H
