#include "eatmemory.h"
#ifdef SYSMEM_MODE_DRAGONFLY
#include <sys/types.h>
#include <sys/sysctl.h>
#include <string.h>

static size_t bytes_to_size_clamped(uint64_t bytes) {
    return bytes > SIZE_MAX ? SIZE_MAX : (size_t)bytes;
}

static size_t pages_to_bytes_clamped(uint64_t pages, uint64_t page_size) {
    if (pages != 0 && page_size > UINT64_MAX / pages) {
        return SIZE_MAX;
    }

    return bytes_to_size_clamped(pages * page_size);
}

static bool read_sysctl_uint64(const char* name, uint64_t* value) {
    unsigned char data[sizeof(uint64_t)] = {0};
    size_t data_size = sizeof(data);

    if (sysctlbyname(name, data, &data_size, NULL, 0) != 0) {
        return false;
    }

    if (data_size == sizeof(uint64_t)) {
        uint64_t result;
        memcpy(&result, data, sizeof(result));
        *value = result;
        return true;
    }

    if (data_size == sizeof(unsigned int)) {
        unsigned int result;
        memcpy(&result, data, sizeof(result));
        *value = result;
        return true;
    }

    if (data_size == sizeof(int)) {
        int result;
        memcpy(&result, data, sizeof(result));
        if (result < 0) {
            return false;
        }

        *value = (uint64_t)result;
        return true;
    }

    return false;
}

void get_system_memory_stats(struct system_memory_stats* stats) {
    uint64_t total_bytes;
    uint64_t page_size;
    uint64_t free_pages;

    stats->supported = false;
    stats->total = 0;
    stats->free = 0;

    if (read_sysctl_uint64("hw.physmem", &total_bytes) &&
        read_sysctl_uint64("hw.pagesize", &page_size) &&
        read_sysctl_uint64("vm.stats.vm.v_free_count", &free_pages) &&
        total_bytes > 0 && page_size > 0) {
        stats->total = bytes_to_size_clamped(total_bytes);
        stats->free = pages_to_bytes_clamped(free_pages, page_size);
        stats->supported = true;
    }
}

#endif
