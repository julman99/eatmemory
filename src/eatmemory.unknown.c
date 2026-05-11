#include "eatmemory.h"
#ifdef SYSMEM_MODE_UNKNOWN

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = false;
}

void get_process_memory_stats(struct process_memory_stats* stats) {
    stats->supported = false;
}

#endif