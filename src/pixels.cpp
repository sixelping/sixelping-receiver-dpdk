//
// Created by rewbycraft on 1/17/20.
//

#include "pixels.h"
#include <png.h>

void setup_pixels(struct app_config *aconf) {
	aconf->pixels.active_buffer = 0;
	aconf->pixels.buf_size = sizeof(uint64_t) * aconf->pixels.width * aconf->pixels.height;
	if (aconf->pixels.buf_size == 0) {
		rte_panic("Zero size buffer allocation!\n");
	}
	aconf->pixels.buffers[0] = static_cast<uint64_t *>(malloc(aconf->pixels.buf_size));
	if (aconf->pixels.buffers[0] == nullptr)
		rte_panic("Unable to allocate pixel buffer 0!\n");
	aconf->pixels.buffers[1] = static_cast<uint64_t *>(malloc(aconf->pixels.buf_size));
	if (aconf->pixels.buffers[1] == nullptr)
		rte_panic("Unable to allocate pixel buffer 1!\n");
	
}

void handle_new_pixel(struct app_config *aconf, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
	if (unlikely(x >= aconf->pixels.width || y >= aconf->pixels.height))
		return;
	uint32_t index = x + y * aconf->pixels.width;
	if (unlikely(index*4 >= aconf->pixels.buf_size))
		return;
	
	uint64_t active_buffer = aconf->pixels.active_buffer;
	uint64_t value = aconf->pixels.buffers[active_buffer % 2][index];
	
	uint16_t aValue = (value >> 48) & 0xFFFF;
	uint16_t rValue = (value >> 32) & 0xFFFF;
	uint16_t gValue = (value >> 16) & 0xFFFF;
	uint16_t bValue = (value >> 0) & 0xFFFF;
	
	aValue++;
	rValue += r;
	gValue += g;
	bValue += b;
	
	value = (((uint64_t)aValue) << 48) | (((uint64_t)rValue) << 32) | (((uint64_t)gValue) << 16) | (((uint64_t)bValue) << 0);
	aconf->pixels.buffers[active_buffer % 2][index] = value;
}

uint64_t *swap_buffers(struct app_config *aconf) {
	uint64_t current_buffer = aconf->pixels.active_buffer % 2;
	uint64_t next_buffer = (current_buffer + 1) % 2;
	
	if (unlikely(aconf->pixels.buffers[current_buffer] == nullptr)) {
		rte_panic("Pixel buffer %lu is null!\n",
		current_buffer);
	}
	
	if (unlikely(aconf->pixels.buffers[next_buffer] == nullptr)) {
		rte_panic("Pixel buffer %lu is null!\n", next_buffer);
	}
	
	if (unlikely(aconf->pixels.buf_size == 0)) {
		rte_panic("Zero size buffer!\n");
	}
	
	//Zero out the next buffer
	explicit_bzero(aconf->pixels.buffers[next_buffer], aconf->pixels.buf_size);
	
	//Increment buffer counter.
	aconf->pixels.active_buffer++;
	
	return aconf->pixels.buffers[current_buffer];
}

std::optional<std::vector<uint8_t>> buffer_to_png(struct app_config *aconf, uint32_t *buffer) {
	png_image image;
	memset(&image, 0, sizeof(png_image));
	image.version = PNG_IMAGE_VERSION;
	image.format = PNG_FORMAT_BGRA;
	image.height = aconf->pixels.height;
	image.width = aconf->pixels.width;
	png_alloc_size_t size = 0;
	
	if (!png_image_write_get_memory_size(image, size, 0, buffer, 0, nullptr)) {
		return std::nullopt;
	}
	
	std::vector<uint8_t> pngbuffer(size);
	png_image_write_to_memory(&image, pngbuffer.data(), &size, 0, buffer, 0, nullptr);
	
	if (size != pngbuffer.size()) {
		return std::nullopt;
	}
	
	return {pngbuffer};
}
