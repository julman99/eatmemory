#include "eatmemory.h"
#ifdef SYSMEM_MODE_WINDOWS
    #include <windows.h>

    void get_system_memory_stats(struct system_memory_stats* stats) {
        MEMORYSTATUSEX memstat;
        memstat.dwLength = sizeof(memstat);

        if (GlobalMemoryStatusEx(&memstat)) {
            stats->supported = true;
            stats->total = (size_t)memstat.ullTotalPhys;
            stats->free = (size_t)memstat.ullAvailPhys;
        } else {
            stats->supported = false;
        }
    }
#endif
