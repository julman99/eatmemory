#include "eatmemory.h"
#ifdef SYSMEM_MODE_LINUX
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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

void get_process_memory_stats(struct process_memory_stats* stats) {
    FILE* file = fopen("/proc/self/status", "r");
    if (file == NULL) {
        stats->supported = false;
        return;
    }
    
    stats->supported = false;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            // Parse VmRSS value in kB
            unsigned long long rss_kb;
            if (sscanf(line + 6, "%llu", &rss_kb) == 1) {
                stats->rss = (size_t)(rss_kb * 1024); // Convert from kB to bytes
                stats->supported = true;
            }
            break;
        }
    }
    fclose(file);
}

#endif