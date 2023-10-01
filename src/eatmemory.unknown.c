#include "eatmemory.h"
#ifdef SYSMEM_MODE_UNKOWN

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = false;
}

#endif