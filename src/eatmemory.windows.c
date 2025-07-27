#include "eatmemory.h"
#ifdef SYSMEM_MODE_WINDOWS
    #include <windows.h>
    #include <psapi.h>

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

    void get_process_memory_stats(struct process_memory_stats* stats) {
        PROCESS_MEMORY_COUNTERS pmc;
        
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            stats->supported = true;
            stats->rss = (size_t)pmc.WorkingSetSize;
        } else {
            stats->supported = false;
        }
    }
#endif 