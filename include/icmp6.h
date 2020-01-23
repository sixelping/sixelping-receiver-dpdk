//
// Created by rewbycraft on 1/17/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_ICMP6_H
#define SIXELPING_RECEIVER_DPDK_ICMP6_H
#include "common.h"

#define ICMP6_PROTO 58

struct icmp6_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
} __attribute__((__packed__));

struct ndp_sollicit_advertise_hdr {
	uint32_t reserved;
	uint8_t addr[16];
} __attribute__((__packed__));

struct ndp_tll_opt_hdr {
	uint8_t type;
	uint8_t length;
	struct rte_ether_addr addr;
} __attribute__((__packed__));

#endif //SIXELPING_RECEIVER_DPDK_ICMP6_H
