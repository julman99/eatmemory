#ifndef eatmemory_h
#define eatmemory_h

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "errors.h"

#define TO_KB 1024UL
#define TO_MB (1024UL * TO_KB)
#define TO_GB (1024UL * TO_MB)

struct system_memory_stats {
    size_t total;
    size_t free;
};

struct eatmemory_backend {
    bool memory_stats_supported;
    bool memory_lock_supported;
};

enum eatmemory_lock_result {
    EM_LOCK_OK = 0,
    EM_LOCK_UNSUPPORTED = 1,
    EM_LOCK_FAILED = 2
};

//system memory stats
void eatmemory_init(struct eatmemory_backend* backend);
void eatmemory_get_system_memory_stats(struct system_memory_stats* stats);
enum eatmemory_lock_result eatmemory_lock_region(void *ptr, size_t size);
void eatmemory_unlock_region(void *ptr, size_t size);

//mem string parsing
size_t string_to_bytes(char * str, eatmemory_error * error);
char * bytes_to_string(size_t bytes, char * str);

// Produces a fresh anonymous stack buffer sized for any bytes_to_string output.
// Each expansion yields a distinct buffer, so the macro is safe to use multiple
// times in the same expression (e.g., two arguments to one printf call).
#define BYTES_TMP_STR() ((char[64]){0})

//mem allocation
struct allocation {
    uint8_t** chunks;
    size_t count;
    size_t total;
    size_t chunk_size;
    size_t locked_count;
};

size_t get_auto_chunk_size(size_t bytes);
size_t get_chunk_size(size_t total, size_t chunk_size, size_t chunk_index);
struct allocation eat(size_t total, size_t chunk_size, eatmemory_error* error);
struct allocation eat_with_options(size_t total, size_t chunk_size, bool lock_memory, eatmemory_error* error);
void digest(struct allocation alloc);
#endif
