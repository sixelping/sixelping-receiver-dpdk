//
// Created by rewbycraft on 1/17/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_APPLICATION_CONFIGURATION_H
#define SIXELPING_RECEIVER_DPDK_APPLICATION_CONFIGURATION_H
#include "common.h"
#include <atomic>
#include <grpc++/grpc++.h>
#include <sixelping-command.grpc.pb.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>

struct app_prometheus_config {
	std::string listen;
	std::shared_ptr<prometheus::Exposer> exposer;
	std::shared_ptr<prometheus::Registry> registry;
};

struct app_ethernet_config {
	uint8_t port_id;
	uint16_t first_rx_queue_id;
	uint16_t first_tx_queue_id;
	uint16_t nb_rx_queues;
	uint16_t nb_tx_queues;
	uint16_t rx_descs;
	uint16_t tx_descs;
	std::string formatted_mac;
};

struct app_pixels_config {
	std::atomic_uint64_t active_buffer;
	uint32_t *buffers[2];
	uint64_t buf_size;
	uint16_t width;
	uint16_t height;
	uint16_t fps;
};

struct app_grpc_config {
	std::unique_ptr<SixelpingRenderer::Stub> grpc_stub;
	std::string host;
};

struct app_processing_config {
	uint16_t rx_burst_size;
	uint16_t process_burst_size;
};

struct app_config {
	struct app_ethernet_config ethdev;
	struct app_pixels_config pixels;
	struct app_processing_config app;
	struct app_grpc_config grpc;
	struct app_prometheus_config prom;
	struct rte_mempool *mbuf_pool;
	uint8_t ipv6_address[16];
	uint8_t ipv6_icmp_prefix[16];
	uint8_t ipv6_icmp_prefix_cidr;
};

#endif //SIXELPING_RECEIVER_DPDK_APPLICATION_CONFIGURATION_H
