#include "eatmemory.h"
#include <sys/mman.h>

enum eatmemory_lock_result eatmemory_lock_region(void *ptr, size_t size) {
    if (mlock(ptr, size) == 0) {
        return EM_LOCK_OK;
    }

    return EM_LOCK_FAILED;
}

void eatmemory_unlock_region(void *ptr, size_t size) {
    munlock(ptr, size);
}
