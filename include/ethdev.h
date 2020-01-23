//
// Created by rewbycraft on 1/17/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_ETHDEV_H
#define SIXELPING_RECEIVER_DPDK_ETHDEV_H
#include "application_configuration.h"

struct eth_stats {
	uint64_t ipackets, opackets, mpackets;
	uint64_t ipacketsps, opacketsps, mpacketsps;
	rte_ether_addr addr;
};

void probe_ethernet_device(struct app_config *aconf);
void setup_ethernet_device(struct app_config *aconf);
struct eth_stats print_stats(struct app_config *aconf);
uint8_t determine_ethernet_port_id();

#endif //SIXELPING_RECEIVER_DPDK_ETHDEV_H
