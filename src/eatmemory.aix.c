#include "eatmemory.h"
#ifdef SYSMEM_MODE_AIX
#include <libperfstat.h>

#define AIX_PERFSTAT_PAGE_SIZE 4096ULL

static size_t pages_4k_to_bytes_clamped(u_longlong_t pages) {
    if (pages > (u_longlong_t)(SIZE_MAX / AIX_PERFSTAT_PAGE_SIZE)) {
        return SIZE_MAX;
    }

    return (size_t)(pages * AIX_PERFSTAT_PAGE_SIZE);
}

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = false;
    stats->total = 0;
    stats->free = 0;

    perfstat_memory_total_t memory;
    int count = perfstat_memory_total(NULL, &memory, sizeof(memory), 1);
    if (count != 1) {
        return;
    }

    stats->total = pages_4k_to_bytes_clamped(memory.real_total);
    stats->free = pages_4k_to_bytes_clamped(memory.real_free);
    stats->supported = true;
}

#endif
