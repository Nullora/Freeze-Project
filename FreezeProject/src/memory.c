#include <stdint.h>

uint32_t total_memory = 16 * 1024 * 1024;
uint32_t used_memory = 0;

static uint32_t heap_top = 0x100000;

void* kmalloc(uint32_t size) {
    void* addr = (void*)heap_top;
    heap_top += size;
    used_memory += size;
    return addr;
}
