#include "eatmemory.h"
#ifdef SYSMEM_MODE_NETBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#include <uvm/uvm_param.h>
#include <uvm/uvm_extern.h>

static size_t pages_to_bytes_clamped(int64_t pages, int64_t page_size) {
    uint64_t unsigned_pages = (uint64_t)pages;
    uint64_t unsigned_page_size = (uint64_t)page_size;

    if (unsigned_pages != 0 && unsigned_page_size > UINT64_MAX / unsigned_pages) {
        return SIZE_MAX;
    }

    uint64_t bytes = unsigned_pages * unsigned_page_size;
    return bytes > SIZE_MAX ? SIZE_MAX : (size_t)bytes;
}

void get_system_memory_stats(struct system_memory_stats* stats) {
    int mib[2] = {CTL_VM, VM_UVMEXP2};
    struct uvmexp_sysctl uvm;
    size_t uvm_size = sizeof(uvm);

    stats->supported = false;
    stats->total = 0;
    stats->free = 0;

    if (sysctl(mib, 2, &uvm, &uvm_size, NULL, 0) == 0 &&
        uvm_size == sizeof(uvm) &&
        uvm.npages > 0 && uvm.pagesize > 0 && uvm.free >= 0) {
        stats->total = pages_to_bytes_clamped(uvm.npages, uvm.pagesize);
        stats->free = pages_to_bytes_clamped(uvm.free, uvm.pagesize);
        stats->supported = true;
    }
}

#endif
