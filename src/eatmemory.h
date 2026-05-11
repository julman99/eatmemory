#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "errors.h"

#ifndef eatmemory_h
#define eatmemory_h

#define TO_KB 1024UL
#define TO_MB (1024UL * TO_KB)
#define TO_GB (1024UL * TO_MB)

// Minimum allocation size for memory verification - below this threshold, 
// OS memory measurement is too imprecise due to page granularity and malloc overhead
#define MIN_VERIFICATION_THRESHOLD_BYTES 1UL * TO_MB  // 64 KB in bytes

#ifdef __APPLE__
    #define SYSMEM_MODE_APPLE
#elif defined(_WIN32) || defined(_WIN64)
    #define SYSMEM_MODE_WINDOWS
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

struct process_memory_stats {
    bool supported;
    size_t rss; // Resident Set Size - actual physical memory used by the process
};

//system memory stats
void get_system_memory_stats(struct system_memory_stats* stats);

//process memory stats  
void get_process_memory_stats(struct process_memory_stats* stats);

//mem string parsing
size_t string_to_bytes(char * str, eatmemory_error * error);
char * bytes_to_string(size_t bytes, char * str);

//mem allocation
size_t get_auto_chunk_size(size_t bytes);
int8_t** eat(size_t total, size_t chunk, eatmemory_error* error);
void digest(int8_t** eaten, size_t total, size_t chunk);
#endif