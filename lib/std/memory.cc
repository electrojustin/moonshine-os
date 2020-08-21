#include <stddef.h>
#include <stdint.h>

#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "arch/interrupts/control.h"

namespace lib {
namespace std {

namespace {
	struct __attribute__((__packed__)) chunk_header {
		uint8_t allocated;
		struct chunk_header* prev;
		struct chunk_header* next;
		size_t size; // Size of the chunk, including the header.
		uint32_t checksum;
	};
	struct chunk_header* next_free_chunk = nullptr;

	uint32_t checksum(struct chunk_header* header) {
		return 0xDEADBEEF + (uint32_t)header->prev + (uint32_t)header->next + header->size + header->allocated;
	}

	// Creates a new chunk, leaving the previous one with offset + sizeof(struct chunk_header) bytes.
	struct chunk_header* insert_chunk(struct chunk_header* prev, size_t offset) {
		struct chunk_header* new_chunk = (struct chunk_header*)((char*)prev + sizeof(struct chunk_header) + offset);
		new_chunk->size = prev->size - sizeof(struct chunk_header) - offset;
		new_chunk->allocated = 0;
		new_chunk->prev = prev;
		new_chunk->next = prev->next;
		new_chunk->next->prev = new_chunk;
		prev->size = offset + sizeof(struct chunk_header);
		prev->next = new_chunk;
		prev->checksum = checksum(prev);
		new_chunk->checksum = checksum(new_chunk);
		new_chunk->next->checksum = checksum(new_chunk->next);
		return new_chunk;
	}

	void delete_chunk(struct chunk_header* to_delete) {
		if (to_delete->prev == to_delete || to_delete->next == to_delete) {
			panic("Cannot delete original chunk!");
		}
		if (to_delete->allocated) {
			panic("Cannot delete allocated chunk!");
		}

		to_delete->prev->size += to_delete->size;
		to_delete->prev->next = to_delete->next;
		to_delete->next->prev = to_delete->prev;
		to_delete->prev->checksum = checksum(to_delete->prev);
		to_delete->next->checksum = checksum(to_delete->next);
	}

	struct chunk_header* find_free_allocation(size_t request) {
		if (!next_free_chunk) {
			panic("Not enough free memory!");
		}

		if (!next_free_chunk->allocated && next_free_chunk->size > request + sizeof(struct chunk_header)) {
			return next_free_chunk;
		}

		struct chunk_header* current = next_free_chunk->next;
		while (current != next_free_chunk) {
			if (current->checksum != checksum(current)) {
				panic("Heap corruption detected!");
			}
			if (!current->allocated && current->size > request + sizeof(struct chunk_header)) {
				return current;
			} else {
				current = current->next;
			}
		}

		panic("Not enough free memory!");
		return nullptr;
	}
}

char initialize_allocator(void* bottom, void* top) {
	if ((uint32_t)top < END_OF_KERNEL - 4*sizeof(struct chunk_header)) {
		return 1;
	}
	if (END_OF_KERNEL > (uint32_t)bottom) {
		bottom = (void*)END_OF_KERNEL;
	}
	if (!next_free_chunk) {
		next_free_chunk = (struct chunk_header*)bottom;
		next_free_chunk->allocated = 0;
		next_free_chunk->prev = next_free_chunk;
		next_free_chunk->next = next_free_chunk;
		next_free_chunk->size = (uint32_t)top - (uint32_t)bottom;
		next_free_chunk->checksum = checksum(next_free_chunk);
	} else {
		struct chunk_header* new_chunk = (struct chunk_header*)bottom;
		new_chunk->allocated = 0;
		new_chunk->size = (uint32_t)top - (uint32_t)bottom;

		if (next_free_chunk->next == next_free_chunk) {
			next_free_chunk->next = new_chunk;
			next_free_chunk->prev = new_chunk;
			new_chunk->next = next_free_chunk;
			new_chunk->prev = next_free_chunk;
			next_free_chunk->checksum = checksum(next_free_chunk);
			new_chunk->checksum = checksum(new_chunk);
		} else {
			struct chunk_header* iter = next_free_chunk;
			while ((uint32_t)iter->next > (uint32_t)iter) {
				if ((uint32_t)iter < (uint32_t)new_chunk &&
				    (uint32_t)iter->next > (uint32_t)new_chunk) {
					if ((uint32_t)iter->next > (uint32_t)new_chunk + new_chunk->size &&
					    (uint32_t)iter + iter->size < (uint32_t)new_chunk) {
						break;
					} else {
						panic("Attempted to map overlapping memory regions!");
					}
				}
			}
			new_chunk->next = iter->next;
			new_chunk->prev = iter;
			iter->next->prev = new_chunk;
			iter->next = new_chunk;
			if ((uint32_t)iter < (uint32_t)next_free_chunk) {
				next_free_chunk = iter;
			}
		}
	}
	return 0;
}

void* kmalloc(size_t size) {
	arch::interrupts::disable_interrupts();

	struct chunk_header* allocation = find_free_allocation(size);
	if (checksum(allocation) != allocation->checksum) {
		panic("Heap corruption detected!");
	}

	allocation->allocated = 1;

	if (allocation->size > size + 4*sizeof(struct chunk_header)) {
		insert_chunk(allocation, size);
	} else {
		allocation->checksum = checksum(allocation);
	}

	if (next_free_chunk == allocation) {
		next_free_chunk = find_free_allocation(0);
	}

	return allocation + 1;
}

void* kmalloc_aligned(size_t size, uint32_t alignment) {
	arch::interrupts::disable_interrupts();

	// We over-allocated by alignment + 2*sizeof(struct chunk_header) to guarantee we will have enough memory.
	struct chunk_header* allocation = (struct chunk_header*)kmalloc(size + alignment + 2*sizeof(struct chunk_header)) - 1;

	// Insert a chunk so that's it's aligned to our boundary.
	// We need an offset such that:
	// (allocation_addr + sizeof(struct chunk_header) + offset + sizeof(struct chunk_header)) % alignment = 0
	insert_chunk(allocation, alignment - (((uint32_t)allocation + 2*sizeof(struct chunk_header)) % alignment));
	struct chunk_header* aligned_allocation = allocation->next;
	aligned_allocation->allocated = 1;
	allocation->checksum = checksum(allocation);
	aligned_allocation->checksum = checksum(aligned_allocation);

	kfree(allocation+1); // The initial allocation shouldn't be needed anymore, recycle it.

	return aligned_allocation + 1;
}

void kfree(void* alloc) {
	arch::interrupts::disable_interrupts();

	struct chunk_header* to_free = (struct chunk_header*)alloc - 1;

	if (!to_free->allocated) {
		panic("Double free detected!");
	}
	if (to_free->checksum != checksum(to_free)) {
		panic("Heap corruption detected!");
	}

	to_free->allocated = 0;
	to_free->checksum = checksum(to_free);

	if (to_free->next == to_free) {
		return;
	} else if ((uint32_t)next_free_chunk > (uint32_t)to_free) {
		next_free_chunk = to_free;
	} else if (!to_free->prev->allocated && 
		   ((uint32_t)to_free->prev) + to_free->prev->size == (uint32_t)to_free) {
		delete_chunk(to_free);
	} else if ((uint32_t)to_free < (uint32_t)next_free_chunk) {
		next_free_chunk = to_free;
	}

	if ((uint32_t)to_free < (uint32_t)to_free->next && !to_free->next->allocated && 
	    (uint32_t)to_free + to_free->size == (uint32_t)to_free->next) {
		delete_chunk(to_free->next);
	}
}

void* krealloc(void* old_alloc, size_t new_size) {
	struct chunk_header* old_header = (struct chunk_header*)old_alloc - 1;
	void* new_alloc = kmalloc(new_size);
	struct chunk_header* new_header = (struct chunk_header*)new_alloc - 1;
	size_t old_data_size = old_header->size - sizeof(struct chunk_header);
	size_t copy_size = old_data_size < new_size ? old_data_size : new_size;
	memcpy((char*)old_alloc, (char*)new_alloc, copy_size);
	kfree(old_alloc);
	return new_alloc;
}

struct mem_stats get_mem_stats(void) {
	struct mem_stats ret = {0, 0, 1};

	ret.free_memory += next_free_chunk->size;
	struct chunk_header* current = next_free_chunk->next;
	while (current != next_free_chunk) {
		if (current->checksum != checksum(current)) {
			panic("Heap corruption detected!");
		}
		if (current->allocated) {
			ret.allocated_memory += current->size;
		} else {
			ret.free_memory += current->size;
		}
		current = current->next;
		ret.num_chunks++;
	}

	return ret;
}

void memcpy(char* src, char* dest, size_t size) {
	for (size_t i = 0; i < size; i++) {
		dest[i] = src[i];
	}
}

void memset(char* to_set, size_t size, char value) {
	for (size_t i = 0; i < size; i++) {
		to_set[i] = value;
	}
}

} // namespace std
} // namespace lib
