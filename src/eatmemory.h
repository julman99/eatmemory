#ifndef eatmemory_h
#define eatmemory_h

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "errors.h"

#define TO_KB 1024UL
#define TO_MB (1024UL * TO_KB)
#define TO_GB (1024UL * TO_MB)

#ifdef __APPLE__
    #define SYSMEM_MODE_APPLE
#elif defined(_WIN32) || defined(_WIN64)
    #define SYSMEM_MODE_WINDOWS
#elif defined(_AIX)
    #define SYSMEM_MODE_AIX
#elif defined(__linux__)
    #define SYSMEM_MODE_LINUX
#else
    #define SYSMEM_MODE_UNKNOWN
#endif


struct system_memory_stats {
    bool supported;
    size_t total;
    size_t free;
};

//system memory stats
void get_system_memory_stats(struct system_memory_stats* stats);

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
};

size_t get_auto_chunk_size(size_t bytes);
struct allocation eat(size_t total, size_t chunk_size, eatmemory_error* error);
void digest(struct allocation alloc);
#endif
