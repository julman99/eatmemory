#include "eatmemory.h"
#ifdef SYSMEM_MODE_LINUX
#include <unistd.h>

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = false;

#if defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGE_SIZE)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    long free_pages = sysconf(_SC_AVPHYS_PAGES);

    if (pages > 0 && page_size > 0 && free_pages > 0) {
        stats->total = (size_t)pages * (size_t)page_size;
        stats->free = (size_t)free_pages * (size_t)page_size;
        stats->supported = true;
    }
#endif
}

#endif