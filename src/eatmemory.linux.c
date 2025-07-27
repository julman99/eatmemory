#include "eatmemory.h"
#ifdef SYSMEM_MODE_LINUX
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = true;
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    stats->total = pages * page_size;

    long free_pages = sysconf(_SC_AVPHYS_PAGES);
    stats->free = free_pages * page_size;
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
            size_t rss_kb;
            if (sscanf(line + 6, "%zu", &rss_kb) == 1) {
                stats->rss = rss_kb * 1024; // Convert from kB to bytes
                stats->supported = true;
            }
            break;
        }
    }
    fclose(file);
}

#endif