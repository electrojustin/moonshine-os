#ifndef LIB_STD_MEMORY_H
#define LIB_STD_MEMORY_H

#include <stdint.h>
#include <stddef.h>

namespace lib {
namespace std {

// Returns 0 if memory was actually being used. This will fail for low memory addresses to protect the kernel.
char initialize_allocator(void* bottom, void* top);

void* kmalloc(size_t size);

void* kmalloc_aligned(size_t size, uint32_t alignment);

void kfree(void* to_free);

void* krealloc(void* old_alloc, size_t new_size);

struct mem_stats {
	size_t free_memory;
	size_t allocated_memory;
	size_t num_chunks;
};

// Assuming 8MB kernel.
constexpr uint32_t END_OF_KERNEL = 8*1024*1024;

struct mem_stats get_mem_stats(void);

void memcpy(char* src, char* dest, size_t size);

void memset(char* to_set, size_t size, char value);

} // namespace std
} // namespace lib

#endif
