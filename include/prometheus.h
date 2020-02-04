//
// Created by rewbycraft on 2/4/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_PROMETHEUS_H
#define SIXELPING_RECEIVER_DPDK_PROMETHEUS_H

#include "application_configuration.h"

void setup_prometheus(struct app_config *aconf);
void record_ping(struct app_config *aconf, std::string source_address);

#endif //SIXELPING_RECEIVER_DPDK_PROMETHEUS_H
