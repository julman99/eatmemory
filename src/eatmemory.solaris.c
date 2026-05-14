#include "eatmemory.h"
#include <unistd.h>

static size_t pages_to_bytes_clamped(long pages, long page_size) {
    uint64_t bytes = (uint64_t)pages * (uint64_t)page_size;
    return bytes > SIZE_MAX ? SIZE_MAX : (size_t)bytes;
}

void eatmemory_init(struct eatmemory_backend* backend) {
    backend->memory_stats_supported = true;
    backend->memory_lock_supported = true;
}

void eatmemory_get_system_memory_stats(struct system_memory_stats* stats) {
    stats->total = 0;
    stats->free = 0;

    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    long free_pages = sysconf(_SC_AVPHYS_PAGES);

    if (pages > 0 && page_size > 0 && free_pages >= 0) {
        stats->total = pages_to_bytes_clamped(pages, page_size);
        stats->free = pages_to_bytes_clamped(free_pages, page_size);
    }
}

#include "eatmemory.posix-common.c"
