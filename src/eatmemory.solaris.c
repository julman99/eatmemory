#include "eatmemory.h"
#ifdef SYSMEM_MODE_SOLARIS
#include <unistd.h>

static size_t pages_to_bytes_clamped(long pages, long page_size) {
    uint64_t bytes = (uint64_t)pages * (uint64_t)page_size;
    return bytes > SIZE_MAX ? SIZE_MAX : (size_t)bytes;
}

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = false;
    stats->total = 0;
    stats->free = 0;

#if defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGE_SIZE)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    long free_pages = sysconf(_SC_AVPHYS_PAGES);

    if (pages > 0 && page_size > 0 && free_pages >= 0) {
        stats->total = pages_to_bytes_clamped(pages, page_size);
        stats->free = pages_to_bytes_clamped(free_pages, page_size);
        stats->supported = true;
    }
#endif
}

#endif
