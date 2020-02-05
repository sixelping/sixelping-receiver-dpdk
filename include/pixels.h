//
// Created by rewbycraft on 1/17/20.
//

#ifndef SIXELPING_RECEIVER_DPDK_PIXELS_H
#define SIXELPING_RECEIVER_DPDK_PIXELS_H

#include "common.h"
#include "application_configuration.h"
#include <vector>
#include <optional>

void setup_pixels(struct app_config *aconf);
void handle_new_pixel(struct app_config *aconf, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
uint64_t *swap_buffers(struct app_config *aconf);
std::optional<std::vector<uint8_t>> buffer_to_png(struct app_config *aconf, uint32_t* buffer);

#endif //SIXELPING_RECEIVER_DPDK_PIXELS_H
