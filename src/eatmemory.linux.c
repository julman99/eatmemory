#include "eatmemory.h"
#ifdef SYSMEM_MODE_LINUX
#include <stdio.h>
#include <unistd.h>

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = true;
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    stats->total = pages * page_size;

    long free_pages = sysconf(_SC_AVPHYS_PAGES);
    stats->free = free_pages * page_size;
}

#endif