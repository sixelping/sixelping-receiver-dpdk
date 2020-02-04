//
// Created by rewbycraft on 1/17/20.
//
#include "../include/packets.h"
#include "../include/icmp6.h"
#include "pixels.h"
#include "prometheus.h"

inline void
process_packet_icmp_neighbor_sollicit(struct app_config *aconf, struct rte_mbuf *pkt, size_t offset, struct rte_ether_hdr *eth, struct rte_ipv6_hdr *ip, struct icmp6_hdr *icmp,
                                      struct ndp_sollicit_advertise_hdr *ndp) {
	//Check that the sollicit is for us.
	if (!memcmp(ndp->addr, aconf->ipv6_address, 16 * sizeof(uint8_t))) {
		//Construct a new packet.
		
		struct rte_mbuf *newpkt = rte_pktmbuf_alloc(aconf->mbuf_pool);
		auto *neweth = (struct rte_ether_hdr *) rte_pktmbuf_append(newpkt, sizeof(struct rte_ether_hdr));
		auto *newip = (struct rte_ipv6_hdr *) rte_pktmbuf_append(newpkt, sizeof(struct rte_ipv6_hdr));
		auto *newicmp = (struct icmp6_hdr *) rte_pktmbuf_append(newpkt, sizeof(struct icmp6_hdr));
		auto *newndp = (struct ndp_sollicit_advertise_hdr *) rte_pktmbuf_append(newpkt, sizeof(struct ndp_sollicit_advertise_hdr));
		auto *newtll = (struct ndp_tll_opt_hdr *) rte_pktmbuf_append(newpkt, sizeof(struct ndp_tll_opt_hdr));
		
		//Null everything
		memset(neweth, 0, sizeof(struct rte_ether_hdr));
		memset(newip, 0, sizeof(struct rte_ipv6_hdr));
		memset(newicmp, 0, sizeof(struct icmp6_hdr));
		memset(newndp, 0, sizeof(struct ndp_sollicit_advertise_hdr));
		memset(newtll, 0, sizeof(struct ndp_tll_opt_hdr));
		
		//Setup mac addresses in L2 header
		rte_ether_addr_copy(&eth->s_addr, &neweth->d_addr);
		rte_eth_macaddr_get(0, &neweth->s_addr);
		neweth->ether_type = htons(RTE_ETHER_TYPE_IPV6);
		
		//Setup ip addresses and proto in L3 header
		memcpy(newip->src_addr, aconf->ipv6_address, 16 * sizeof(uint8_t));
		memcpy(newip->dst_addr, ip->src_addr, 16 * sizeof(uint8_t));
		newip->proto = ICMP6_PROTO;
		newip->hop_limits = 0xff;
		newip->vtc_flow = htonl(0x60000000); //IPv6 header magic
		//Compute length of payload
		newip->payload_len = htons(sizeof(struct icmp6_hdr) + sizeof(struct ndp_sollicit_advertise_hdr) + sizeof(struct ndp_tll_opt_hdr));
		
		//Setup ICMP header
		newicmp->type = 136; //NDP Advertisement
		
		//Setup ndp fields.
		//Set flags
		newndp->reserved = htonl(0b01100000000000000000000000000000);
		//Copy addr to reply.
		memcpy(newndp->addr, ndp->addr, 16 * sizeof(uint8_t));
		
		//Setup the target link layer option.
		newtll->type = 2;
		newtll->length = 1;
		rte_eth_macaddr_get(0, &newtll->addr);
		
		//Compute icmp6 checksum.
		//This reimplements rte_ipv6_udptcp_cksum to allow multiple headers to be processed easily.
		struct temp_t {
			struct icmp6_hdr icmp;
			struct ndp_sollicit_advertise_hdr ndp;
			struct ndp_tll_opt_hdr tll;
		} __attribute__((__packed__));
		
		struct temp_t *entireicmp = static_cast<temp_t *>(rte_malloc("ICMP6PKT", sizeof(struct temp_t), 0));
		memcpy(&entireicmp->icmp, newicmp, sizeof(struct icmp6_hdr));
		memcpy(&entireicmp->ndp, newndp, sizeof(struct ndp_sollicit_advertise_hdr));
		memcpy(&entireicmp->tll, newtll, sizeof(struct ndp_tll_opt_hdr));
		newicmp->checksum = rte_ipv6_udptcp_cksum(newip, entireicmp);
		rte_free(entireicmp);
		
		if (0) {
			uint32_t cksum;
			
			cksum = rte_ipv6_phdr_cksum(ip, 0);
			cksum += rte_raw_cksum(newicmp, rte_be_to_cpu_16(sizeof(struct icmp6_hdr)));
			cksum += rte_raw_cksum(newndp, rte_be_to_cpu_16(sizeof(struct ndp_sollicit_advertise_hdr)));
			cksum += rte_raw_cksum(newtll, rte_be_to_cpu_16(sizeof(struct ndp_tll_opt_hdr)));
			cksum = __rte_raw_cksum_reduce(cksum);
			
			cksum = ((cksum & 0xffff0000) >> 16) + (cksum & 0xffff);
			cksum = (~cksum) & 0xffff;
			if (cksum == 0)
				cksum = 0xffff;
			
			newicmp->checksum = (uint16_t) cksum;
		}
		
		if (!rte_eth_tx_burst(aconf->ethdev.port_id, aconf->ethdev.first_tx_queue_id, &newpkt, 1)) {
			RTE_LOG(WARNING, APP, "Failed to send NDP packet.\n");
		}
	}
}

void _format_ipv6_addr(char* buf, size_t size, uint8_t* addr) {
	snprintf(buf, size, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
	         addr[0], addr[1], addr[2], addr[3],
	         addr[4], addr[5], addr[6], addr[7],
	         addr[8], addr[9], addr[10], addr[11],
	         addr[12], addr[13], addr[14], addr[15]
	);
}

std::string format_ipv6_addr(uint8_t *addr) {
	char str[64];
	_format_ipv6_addr(str, 64, addr);
	return std::string(str);
}


inline void
process_packet_icmp_echo_request(struct app_config *aconf, struct rte_mbuf *pkt, size_t offset, struct rte_ether_hdr *eth, struct rte_ipv6_hdr *ip, struct icmp6_hdr *icmp) {
	//Drop overly big packets. No need to let people flood us with bandwidth.
	if (rte_pktmbuf_pkt_len(pkt) > 200)
		return;
	
	uint8_t i, fb = aconf->ipv6_icmp_prefix_cidr / 8;
	for (i = 0; i < fb && i < 15; i++) {
		if (unlikely(ip->dst_addr[i] != aconf->ipv6_icmp_prefix[i])) {
			return;
		}
	}
	
	uint8_t l = aconf->ipv6_icmp_prefix_cidr - (i * 8);
	uint8_t mask = 0;
	for (uint8_t j = 0; j < 8; j++) {
		mask <<= 1;
		mask |= j < l;
	}
	
	if (unlikely((ip->dst_addr[i] & mask) != (aconf->ipv6_icmp_prefix[i] & mask))) {
		return;
	}
	
	uint16_t x = (ip->dst_addr[7] & 0xF) + ((ip->dst_addr[7] & 0xF0) >> 4) * 10 + (ip->dst_addr[6] & 0xF) * 100 + ((ip->dst_addr[6] & 0xF0) >> 4) * 1000;
	uint16_t y = (ip->dst_addr[9] & 0xF) + ((ip->dst_addr[9] & 0xF0) >> 4) * 10 + (ip->dst_addr[8] & 0xF) * 100 + ((ip->dst_addr[8] & 0xF0) >> 4) * 1000;
	uint8_t r = ip->dst_addr[11], g = ip->dst_addr[13], b = ip->dst_addr[15];
	
	handle_new_pixel(aconf, x, y, r, g, b);
	record_ping(aconf, format_ipv6_addr(ip->src_addr));
}

inline void process_packet_icmp(struct app_config *aconf, struct rte_mbuf *pkt, size_t offset, struct rte_ether_hdr *eth, struct rte_ipv6_hdr *ip, struct icmp6_hdr *icmp) {
	if (icmp->type == 135 && icmp->code == 0) { //If packet is NDP sollicit
		if (rte_pktmbuf_pkt_len(pkt) - offset >= sizeof(struct ndp_sollicit_advertise_hdr)) {
			struct ndp_sollicit_advertise_hdr *ndp = rte_pktmbuf_mtod_offset(pkt, struct ndp_sollicit_advertise_hdr*, offset);
			process_packet_icmp_neighbor_sollicit(aconf, pkt, offset + sizeof(struct ndp_sollicit_advertise_hdr), eth, ip, icmp, ndp);
		}
	}
	
	if (icmp->type == 128 && icmp->code == 0) { //If packet is ICMP Echo Request
		process_packet_icmp_echo_request(aconf, pkt, offset, eth, ip, icmp);
	}
}

inline void process_packet_l3(struct app_config *aconf, struct rte_mbuf *pkt, size_t offset, struct rte_ether_hdr *eth, struct rte_ipv6_hdr *ip) {
	if (ip->proto == ICMP6_PROTO) {
		if (rte_pktmbuf_pkt_len(pkt) - offset >= sizeof(struct icmp6_hdr)) {
			struct icmp6_hdr *icmp = rte_pktmbuf_mtod_offset(pkt, struct icmp6_hdr*, offset);
			process_packet_icmp(aconf, pkt, offset + sizeof(struct icmp6_hdr), eth, ip, icmp);
		}
	}
	
}

inline void process_packet_l2(struct app_config *aconf, struct rte_mbuf *pkt, size_t offset, struct rte_ether_hdr *eth) {
	if (ntohs(eth->ether_type) == RTE_ETHER_TYPE_IPV6) {
		if (rte_pktmbuf_pkt_len(pkt) - offset >= sizeof(struct rte_ipv6_hdr)) {
			struct rte_ipv6_hdr *ip = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv6_hdr*, offset);
			process_packet_l3(aconf, pkt, offset + sizeof(struct rte_ipv6_hdr), eth, ip);
		}
	}
	
}

void process_packet(struct app_config *aconf, struct rte_mbuf *pkt) {
	
	if (rte_pktmbuf_pkt_len(pkt) >= sizeof(struct rte_ether_hdr)) {
		struct rte_ether_hdr *eth = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
		process_packet_l2(aconf, pkt, sizeof(struct rte_ether_hdr), eth);
	}
	
}
