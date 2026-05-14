#include "eatmemory.h"
#include <sys/mman.h>
#include <unistd.h>

#ifndef EATMEMORY_MLOCK
#define EATMEMORY_MLOCK mlock
#endif

#ifndef EATMEMORY_MUNLOCK
#define EATMEMORY_MUNLOCK munlock
#endif

#ifndef EATMEMORY_SYSCONF
#define EATMEMORY_SYSCONF sysconf
#endif

struct eatmemory_lock_range {
    void *ptr;
    size_t size;
};

static long eatmemory_get_page_size(void) {
#if defined(_SC_PAGESIZE)
    return EATMEMORY_SYSCONF(_SC_PAGESIZE);
#elif defined(_SC_PAGE_SIZE)
    return EATMEMORY_SYSCONF(_SC_PAGE_SIZE);
#else
    return -1;
#endif
}

static bool eatmemory_get_lock_range(void *ptr, size_t size, struct eatmemory_lock_range *range) {
    long page_size_long = eatmemory_get_page_size();
    if (page_size_long <= 0) {
        return false;
    }

    uintptr_t page_size = (uintptr_t)page_size_long;
    uintptr_t start = (uintptr_t)ptr;
    uintptr_t aligned_start = start - (start % page_size);
    if ((uintptr_t)size > UINTPTR_MAX - start) {
        return false;
    }

    uintptr_t end = start + (uintptr_t)size;
    uintptr_t aligned_end = end;
    uintptr_t end_remainder = end % page_size;
    if (end_remainder != 0) {
        uintptr_t bytes_to_next_page = page_size - end_remainder;
        if (aligned_end > UINTPTR_MAX - bytes_to_next_page) {
            return false;
        }
        aligned_end += bytes_to_next_page;
    }

    uintptr_t aligned_size = aligned_end - aligned_start;
    if (aligned_size > SIZE_MAX) {
        return false;
    }

    range->ptr = (void *)aligned_start;
    range->size = (size_t)aligned_size;
    return true;
}

enum eatmemory_lock_result eatmemory_lock_region(void *ptr, size_t size) {
    if (size == 0) {
        return EM_LOCK_OK;
    }

    struct eatmemory_lock_range range;
    if (!eatmemory_get_lock_range(ptr, size, &range)) {
        return EM_LOCK_FAILED;
    }

    if (EATMEMORY_MLOCK(range.ptr, range.size) == 0) {
        return EM_LOCK_OK;
    }

    return EM_LOCK_FAILED;
}

void eatmemory_unlock_region(void *ptr, size_t size) {
    if (size == 0) {
        return;
    }

    struct eatmemory_lock_range range;
    if (!eatmemory_get_lock_range(ptr, size, &range)) {
        return;
    }

    EATMEMORY_MUNLOCK(range.ptr, range.size);
}
