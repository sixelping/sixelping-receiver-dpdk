//
// Created by rewbycraft on 1/17/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_PACKETS_H
#define SIXELPING_RECEIVER_DPDK_PACKETS_H

#include "common.h"
#include "application_configuration.h"

void process_packet(struct app_config *aconf, struct rte_mbuf *pkt);

#endif //SIXELPING_RECEIVER_DPDK_PACKETS_H
