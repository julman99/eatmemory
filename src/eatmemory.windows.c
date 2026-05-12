#include "eatmemory.h"
#ifdef SYSMEM_MODE_WINDOWS
    #include <windows.h>

    void get_system_memory_stats(struct system_memory_stats* stats) {
        MEMORYSTATUSEX memstat;
        memstat.dwLength = sizeof(memstat);

        if (GlobalMemoryStatusEx(&memstat)) {
            stats->supported = true;
            // ullTotalPhys / ullAvailPhys are DWORDLONG (64-bit). On 32-bit
            // Windows with >4 GB RAM, a direct cast to size_t (32-bit there)
            // silently truncates and reports nonsense memory amounts. Clamp
            // to SIZE_MAX so the caller at least sees a bounded value.
            stats->total = (memstat.ullTotalPhys > SIZE_MAX) ? SIZE_MAX : (size_t)memstat.ullTotalPhys;
            stats->free  = (memstat.ullAvailPhys > SIZE_MAX) ? SIZE_MAX : (size_t)memstat.ullAvailPhys;
        } else {
            stats->supported = false;
        }
    }
#endif
