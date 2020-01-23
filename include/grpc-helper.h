//
// Created by rewbycraft on 1/19/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_GRPC_HELPER_H
#define SIXELPING_RECEIVER_DPDK_GRPC_HELPER_H

#include "common.h"
#include "application_configuration.h"
#include <vector>
#include "ethdev.h"

void setup_grpc(struct app_config *aconf);
void send_frame_update(struct app_config *aconf, std::vector<uint8_t> image);
void send_metrics(struct app_config *aconf, struct eth_stats stats);

#endif //SIXELPING_RECEIVER_DPDK_GRPC_HELPER_H
