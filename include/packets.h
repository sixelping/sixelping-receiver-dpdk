//
// Created by rewbycraft on 1/17/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_PACKETS_H
#define SIXELPING_RECEIVER_DPDK_PACKETS_H

#include "common.h"
#include "application_configuration.h"

void process_packet(struct app_config *aconf, struct rte_mbuf *pkt);
void format_ipv6_addr(char* buf, size_t size, uint8_t* addr);

#endif //SIXELPING_RECEIVER_DPDK_PACKETS_H
