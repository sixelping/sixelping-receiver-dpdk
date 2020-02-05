//
// Created by rewbycraft on 1/19/20.
//

#include "../include/common.h"
#include "../include/application_configuration.h"
#include "../include/ethdev.h"
#include "packets.h"
#include "pixels.h"
#include "grpc-helper.h"
#include "lcoremain.h"

static volatile bool force_quit;

int lcore_main(void *arg) {
	auto *aconf = (struct app_config *) arg;
	auto lcore_id = static_cast<uint16_t>(rte_lcore_id());
	uint16_t lcore_index = static_cast<uint16_t>(rte_lcore_index(lcore_id));
	
	RTE_LOG(INFO, APP, "Starting core %u, index %u.\n", lcore_id, lcore_index);
	if (lcore_index == 0) {
		RTE_LOG(INFO, APP, "Core %u is working the metrics.\n", lcore_id);
		
		using namespace std::chrono_literals;
		std::chrono::time_point lastMetrics = std::chrono::system_clock::now();
		while (!force_quit) {
			std::chrono::time_point now = std::chrono::system_clock::now();
			if (now - lastMetrics >= 1000ms) {
				send_metrics(aconf, print_stats(aconf));
				
				lastMetrics = now;
			}
			rte_delay_ms(100);
		}
	} else if (lcore_index == 1) {
		RTE_LOG(INFO, APP, "Core %u is working the frames.\n", lcore_id);
		
		using namespace std::chrono_literals;
		std::chrono::time_point lastUpdate = std::chrono::system_clock::now();
		std::vector<uint8_t> image(aconf->pixels.width*aconf->pixels.height*4);
		while (!force_quit) {
			std::chrono::time_point now = std::chrono::system_clock::now();
			if (now - lastUpdate >= (1000ms / aconf->pixels.fps)) {
				uint64_t *buffer = swap_buffers(aconf);
				uint8_t *target = image.data();
				for (size_t i = 0; i < aconf->pixels.width*aconf->pixels.height; i++) {
					uint64_t value = buffer[i];
					uint16_t aValue = (value >> 48) & 0xFFFF;
					uint16_t rValue = (value >> 32) & 0xFFFF;
					uint16_t gValue = (value >> 16) & 0xFFFF;
					uint16_t bValue = (value >> 0) & 0xFFFF;
					
					if (aValue > 0) {
						rValue /= aValue;
						gValue /= aValue;
						bValue /= aValue;
						aValue = 0xFF;
					} else {
						rValue = 0x0;
						gValue = 0x0;
						bValue = 0x0;
						aValue = 0x0;
					}
					
					target[4*i + 0] = bValue;
					target[4*i + 1] = gValue;
					target[4*i + 2] = rValue;
					target[4*i + 3] = aValue;
				}
				send_frame_update(aconf, image);
				lastUpdate = now;
			}
			rte_delay_ms(1);
		}
		
	} else if (lcore_index > 1 && lcore_index <= aconf->ethdev.nb_rx_queues) {
		uint16_t queue_id = aconf->ethdev.first_rx_queue_id + lcore_index - 2;
		RTE_LOG(INFO, APP, "Core %u is working RX queue %u.\n", lcore_id, queue_id);
		struct rte_mbuf *bufs[aconf->app.rx_burst_size];
		
		while (!force_quit) {
			//Read packets from ethernet device
			const unsigned nb_rx = rte_eth_rx_burst(aconf->ethdev.port_id, queue_id, bufs, aconf->app.rx_burst_size);
			
			if (unlikely(nb_rx == 0)) {
				//rte_pause();
				continue;
			}
			
			for (uint16_t i = 0; i < nb_rx; i++) {
				process_packet(aconf, bufs[i]);
				nipktmfree(bufs[i]);
			}
			
		}
	}
	
	RTE_LOG(INFO, APP, "Exiting core %u.\n", lcore_id);
	
	return 0;
}

static void signal_handler(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
		       signum);
		force_quit = true;
	}
}

void setup_signal_handler() {
	force_quit = false;
	struct sigaction act = {};
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_flags = SA_RESETHAND;
	act.sa_handler = signal_handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	
}
