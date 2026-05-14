#include "eatmemory.h"

void eatmemory_get_backend_capabilities(struct eatmemory_backend* backend) {
    backend->memory_stats_supported = false;
    backend->memory_lock_supported = false;
}

enum eatmemory_stats_result eatmemory_get_system_memory_stats(struct system_memory_stats* stats) {
    stats->total = 0;
    stats->free = 0;

    return EM_STATS_UNSUPPORTED;
}

enum eatmemory_lock_result eatmemory_lock_region(void *ptr, size_t size) {
    (void)ptr;
    (void)size;

    return EM_LOCK_UNSUPPORTED;
}

void eatmemory_unlock_region(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
}
