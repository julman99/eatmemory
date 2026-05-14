#include "eatmemory.h"

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = false;
    stats->total = 0;
    stats->free = 0;
}
