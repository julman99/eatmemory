#include "eatmemory.h"
#include <windows.h>

void eatmemory_get_backend_capabilities(struct eatmemory_backend* backend) {
    backend->memory_stats_supported = true;
    backend->memory_lock_supported = true;
}

enum eatmemory_stats_result eatmemory_get_system_memory_stats(struct system_memory_stats* stats) {
    stats->total = 0;
    stats->free = 0;

    MEMORYSTATUSEX memstat;
    memstat.dwLength = sizeof(memstat);

    if (GlobalMemoryStatusEx(&memstat)) {
        // ullTotalPhys / ullAvailPhys are DWORDLONG (64-bit). On 32-bit
        // Windows with >4 GB RAM, a direct cast to size_t (32-bit there)
        // silently truncates and reports nonsense memory amounts. Clamp
        // to SIZE_MAX so the caller at least sees a bounded value.
        stats->total = (memstat.ullTotalPhys > SIZE_MAX) ? SIZE_MAX : (size_t)memstat.ullTotalPhys;
        stats->free  = (memstat.ullAvailPhys > SIZE_MAX) ? SIZE_MAX : (size_t)memstat.ullAvailPhys;
        return EM_STATS_OK;
    }

    return EM_STATS_FAILED;
}

enum eatmemory_lock_result eatmemory_lock_region(void *ptr, size_t size) {
    if (VirtualLock(ptr, (SIZE_T)size)) {
        return EM_LOCK_OK;
    }

    return EM_LOCK_FAILED;
}

void eatmemory_unlock_region(void *ptr, size_t size) {
    VirtualUnlock(ptr, (SIZE_T)size);
}
