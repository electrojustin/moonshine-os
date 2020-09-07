#ifndef IO_I386_IO_H
#define IO_I386_IO_H

#include <stddef.h>
#include <stdint.h>

namespace io {

void out(uint16_t port, uint8_t value);

void out16(uint16_t port, uint16_t value);

void out32(uint16_t port, uint32_t value);

void out_block(uint16_t port, uint8_t value, size_t size);

void out_block16(uint16_t port, uint16_t *value, size_t size);

uint8_t in(uint16_t port);

uint16_t in16(uint16_t port);

uint32_t in32(uint16_t port);

void in_block(uint16_t port, uint8_t *buffer, size_t size);

void in_block16(uint16_t port, uint16_t *buffer, size_t size);

} // namespace io

#endif
