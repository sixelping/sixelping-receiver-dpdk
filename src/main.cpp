#include "common.h"
#include "application_configuration.h"
#include "ethdev.h"
#include "pixels.h"
#include "grpc-helper.h"
#include "lcoremain.h"
#include <getopt.h>
#include <arpa/inet.h>

uint16_t hex_to_int(char c) {
	if ('0' <= c && c <= '9') {
		return static_cast<uint8_t>(c - '0');
	}
	if ('a' <= c && c <= 'f') {
		return static_cast<uint8_t>(c - 'a');
	}
	if ('A' <= c && c <= 'F') {
		return static_cast<uint8_t>(c - 'A');
	}
	return 0xFF;
}

void parse_ipv6_address(char *str, uint8_t *addr, uint8_t *cidr) {
	char *sptr = strchr(str, '/');
	size_t len = strlen(str);
	
	if (sptr != nullptr) {
		len = sptr - str;
		sptr++;
		*cidr = static_cast<uint8_t>(std::atoi(sptr));
	}
	char tstr[len + 1];
	strncpy(tstr, str, len);
	tstr[len] = '\0';
	
	struct in6_addr a = {};
	if (inet_pton(AF_INET6, tstr, &a) != 1) {
		rte_panic("Invalid ipv6 address.\n");
	}
	
	memcpy(addr, a.s6_addr, 16 * sizeof(uint8_t));
}

std::string format_v6(uint8_t *addr) {
	char str[64];
	sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7], addr[8],
	        addr[9], addr[10], addr[11], addr[12], addr[13], addr[14], addr[15]);
	return std::string(str);
}

void populate_aconf(struct app_config *aconf, int argc, char *argv[]) {
	static struct option long_options[] = {
		{"address",      required_argument, nullptr, 'a'},
		{"prefix",       required_argument, nullptr, 'p'},
		{"grpc-address", required_argument, nullptr, 'h'},
		{"rxqueues",     required_argument, nullptr, 'r'},
		{"txqueues",     required_argument, nullptr, 't'},
		{0, 0,                              0,       0},
	};
	
	//Default value
	aconf->grpc.host = "localhost:50051";
	
	bool set_addr = false, set_prefix = false, set_host = false;
	int option_index = 0;
	for (;;) {
		int c = getopt_long(argc, argv, "a:p:h:r:t:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 'a':
				parse_ipv6_address(optarg, aconf->ipv6_address, nullptr);
				set_addr = true;
				break;
			case 'p':
				parse_ipv6_address(optarg, aconf->ipv6_icmp_prefix, &aconf->ipv6_icmp_prefix_cidr);
				set_prefix = true;
				break;
			case 'h':
				aconf->grpc.host = std::string(optarg);
				set_host = true;
				break;
			case 'r':
				aconf->ethdev.nb_rx_queues = static_cast<uint16_t>(std::atoi(optarg));
				break;
			case 't':
				aconf->ethdev.nb_tx_queues = static_cast<uint16_t>(std::atoi(optarg));
				break;
			default:
				exit(1);
		}
	}
	
	if (!set_addr || !set_prefix || !set_host) {
		rte_panic("Required parameters are missing!\n");
	}
	
	RTE_LOG(INFO, APP, "Configuration:\n");
	RTE_LOG(INFO, APP, "\tAddress: %s\n", format_v6(aconf->ipv6_address).c_str());
	RTE_LOG(INFO, APP, "\tPrefix: %s/%u\n", format_v6(aconf->ipv6_icmp_prefix).c_str(), aconf->ipv6_icmp_prefix_cidr);
	RTE_LOG(INFO, APP, "\tGRPC Endpoint: %s\n", aconf->grpc.host.c_str());
}

int main(int argc, char *argv[]) {
	int ret;
	struct app_config aconf = {};
	
	printf("APP: Initializing GRPC system (if it hangs here, grpc is fucked)...\n");
	grpc_init();
	printf("APP: Done! \\o/\n");
	
	printf("APP: Initializing DPDK...\n");
	//Initialize DPDK EAL
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("EAL Init failed\n");
	RTE_LOG(INFO, APP, "Done!\n");
	
	argc -= ret;
	argv += ret;
	
	populate_aconf(&aconf, argc, argv);
	
	probe_ethernet_device(&aconf);
	
	unsigned nb_mbufs = RTE_MAX(aconf.ethdev.nb_rx_queues * (aconf.ethdev.rx_descs + aconf.ethdev.tx_descs + aconf.app.rx_burst_size) + rte_lcore_count() * MBUF_CACHE_SIZE,
	                            8192U);
	RTE_LOG(INFO, APP, "Attempting to setup an mbuf pool with %u mbufs...\n", nb_mbufs);
	/* Creates a new mbuf mempool */
	aconf.mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", nb_mbufs, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	
	if (aconf.mbuf_pool == nullptr)
		rte_panic("mbuf_pool create failed: %d\n", rte_errno);
	
	setup_grpc(&aconf);
	
	setup_pixels(&aconf);
	
	setup_ethernet_device(&aconf);
	
	aconf.ring = rte_ring_create("PACKETS", NUM_MBUFS, SOCKET_ID_ANY, (aconf.ethdev.nb_rx_queues <= 1) ? RING_F_SP_ENQ : 0);
	if (aconf.ring == nullptr)
		rte_panic("ring failure");
	
	rte_eal_mp_remote_launch(lcore_main, &aconf, CALL_MASTER);
	
	rte_eal_mp_wait_lcore();
	
	/* There is no un-init for eal */
	
	return 0;
}
