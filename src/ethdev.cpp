//
// Created by rewbycraft on 1/17/20.
//
#include <common.h>
#include "../include/ethdev.h"

void probe_ethernet_device(struct app_config *aconf) {
	RTE_LOG(INFO, APP, "Querying ethernet device...\n");
	//Determine the ethernet port to use
	aconf->ethdev.port_id = determine_ethernet_port_id();
	
	struct rte_eth_dev_info dev_info = {};
	if (rte_eth_dev_info_get(aconf->ethdev.port_id, &dev_info))
		rte_panic("Unable to query ethernet device capabilities!\n");
	aconf->ethdev.first_rx_queue_id = 0;
	aconf->ethdev.first_tx_queue_id = 0;
	if (aconf->ethdev.nb_rx_queues <= 0)
		aconf->ethdev.nb_rx_queues = dev_info.max_rx_queues;
	if (aconf->ethdev.nb_tx_queues <= 0)
		aconf->ethdev.nb_tx_queues = dev_info.max_tx_queues;
	RTE_LOG(INFO, APP, "\tNumber of used RX queues: %d\n", aconf->ethdev.nb_rx_queues);
	RTE_LOG(INFO, APP, "\tNumber of used TX queues: %d\n", aconf->ethdev.nb_tx_queues);
	
	aconf->ethdev.rx_descs = dev_info.rx_desc_lim.nb_max;
	aconf->ethdev.tx_descs = dev_info.tx_desc_lim.nb_max;
	
	if (rte_eth_dev_adjust_nb_rx_tx_desc(aconf->ethdev.port_id, &aconf->ethdev.rx_descs, &aconf->ethdev.tx_descs))
		rte_panic("Cannot adjust rxtx params.\n");
	
	RTE_LOG(INFO, APP, "\tMax nb_rx_descriptors: %u\n", aconf->ethdev.rx_descs);
	RTE_LOG(INFO, APP, "\tMax nb_tx_descriptors: %u\n", aconf->ethdev.tx_descs);
	//Use half the buffer space per poll.
	//This way we shouldn't drop anything when the buffer fills up.
	aconf->app.rx_burst_size = static_cast<uint16_t>(aconf->ethdev.rx_descs / 2);
	aconf->app.process_burst_size = static_cast<uint16_t>(aconf->ethdev.rx_descs / 2);
	//static_cast<uint16_t>(aconf->app.rx_burst_size / std::max((rte_lcore_count() - aconf->ethdev.nb_rx_queues - 1), (unsigned)1));
	
	RTE_LOG(INFO, APP, "\tRX burst size: %d\n", aconf->app.rx_burst_size);
	RTE_LOG(INFO, APP, "\tProcess burst size: %d\n", aconf->app.process_burst_size);
	
	RTE_LOG(INFO, APP, "Done querying ethernet device!\n");
}

void setup_ethernet_device(struct app_config *aconf) {
	RTE_LOG(INFO, APP, "Starting ethernet device...\n");
	
	RTE_LOG(INFO, APP, "\tConfiguring device...\n");
	struct rte_eth_conf port_conf = {};
	port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;
	port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
	int ret;
	uint16_t q;
	
	/* Configure the Ethernet device. */
	ret = rte_eth_dev_configure(aconf->ethdev.port_id, aconf->ethdev.nb_rx_queues, aconf->ethdev.nb_tx_queues, &port_conf);
	if (ret != 0)
		rte_panic("Failed to configure device: %d\n", ret);
	
	RTE_LOG(INFO, APP, "\tSetting up queues...\n");
	
	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = aconf->ethdev.first_rx_queue_id; q < aconf->ethdev.nb_rx_queues; q++) {
		ret = rte_eth_rx_queue_setup(aconf->ethdev.port_id, q, aconf->ethdev.rx_descs, static_cast<unsigned int>(rte_eth_dev_socket_id(aconf->ethdev.port_id)), NULL,
		                             aconf->mbuf_pool);
		if (ret < 0)
			rte_panic("Failed to setup rx queue %d: %d\n", q, ret);
	}
	
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = aconf->ethdev.first_tx_queue_id; q < aconf->ethdev.nb_tx_queues; q++) {
		ret = rte_eth_tx_queue_setup(aconf->ethdev.port_id, q, aconf->ethdev.tx_descs, static_cast<unsigned int>(rte_eth_dev_socket_id(aconf->ethdev.port_id)), NULL);
		if (ret < 0)
			rte_panic("Failed to setup tx queue %d: %d\n", q, ret);
	}
	
	RTE_LOG(INFO, APP, "\tStarting device...\n");
	/* Start the Ethernet port. */
	ret = rte_eth_dev_start(aconf->ethdev.port_id);
	if (ret < 0)
		rte_panic("Failed to start device: %d", ret);
	
	/* Enable RX in promiscuous mode for the Ethernet device. */
	rte_eth_promiscuous_enable(aconf->ethdev.port_id);
	rte_eth_allmulticast_enable(aconf->ethdev.port_id);
	
	RTE_LOG(INFO, APP, "Started ethernet device!\n");
}

struct eth_stats print_stats(struct app_config *aconf) {
	struct rte_eth_stats stats = {};
	struct eth_stats rc = {};
	uint16_t port = aconf->ethdev.port_id;
	static uint64_t old_ipackets = 0, old_opackets = 0, old_imissed = 0;
	static std::chrono::time_point old_time = std::chrono::system_clock::now();
	
	if (rte_eth_stats_get(port, &stats)) {
		RTE_LOG(INFO, APP, "Statistics for port %u: Failed to retrieve statistics.\n", port);
	} else {
		auto now = std::chrono::system_clock::now();
		double deltaT = double(std::chrono::duration_cast<std::chrono::milliseconds>(now - old_time).count() * std::milli::num) / double(std::milli::den);
		rc.ipacketsps = uint64_t(double(stats.ipackets - old_ipackets) / deltaT);
		rc.opacketsps = uint64_t(double(stats.opackets - old_opackets) / deltaT);
		rc.mpacketsps = uint64_t(double(stats.imissed - old_imissed) / deltaT);
		rc.ipackets = stats.ipackets;
		rc.opackets = stats.opackets;
		rc.mpackets = stats.imissed;
		rc.ibytes = stats.ibytes;
		rc.obytes = stats.obytes;
		RTE_LOG(INFO, APP, "Statistics: Rx: %9lu (%9lu pkts/s) Tx: %9lu (%9lu pkts/s) dropped: %9lu (%9lu pkts/s)\n", stats.ipackets, rc.ipacketsps, stats.opackets,
		        rc.opacketsps, rc.mpackets, rc.mpacketsps);
		old_ipackets = stats.ipackets;
		old_opackets = stats.opackets;
		old_imissed = stats.imissed;
		old_time = now;
	}
	
	rte_eth_macaddr_get(port, &rc.addr);
	
	return rc;
}

uint8_t determine_ethernet_port_id() {
	if (rte_eth_dev_count_avail() == 0) {
		rte_panic("No ethernet devices available!\n");
	}
	
	RTE_LOG(INFO, APP, "\tEthernet devices:\n");
	for (uint8_t i = 0; i < rte_eth_dev_count_avail(); i++) {
		char mac[64];
		struct rte_ether_addr addr = {};
		struct rte_eth_link link = {};
		
		if (rte_eth_macaddr_get(i, &addr)) {
			RTE_LOG(INFO, APP, "\t - Device %d: Unable to get mac address.\n", i);
			continue;
		}
		
		rte_ether_format_addr(mac, 64, &addr);
		
		if (rte_eth_link_get(i, &link)) {
			RTE_LOG(INFO, APP, "\t - Device %d: Unable to get link.\n", i);
			continue;
		}
		
		RTE_LOG(INFO, APP, "\t - Device %d:\tMAC: %s\tStatus: %s\tSpeed: %d mbps\tDuplex: %s\n", i, mac,
		        (link.link_status) ? "UP" : "DOWN", link.link_speed,
		        (link.link_duplex) ? "HALF" : "FULL");
	}
	
	if (rte_eth_dev_count_avail() == 1) {
		RTE_LOG(INFO, APP, "\tOnly one ethernet device available, selecting ethernet device 0.\n");
		return 0;
	}
	
	rte_panic("NOT IMPLEMENTED: MULTIPLE NICS, USING NIC 0\n");
	return 0;
}
