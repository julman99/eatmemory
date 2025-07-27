#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "errors.h"

#ifndef eatmemory_h
#define eatmemory_h

#ifdef __APPLE__
    #define SYSMEM_MODE_APPLE
#elif defined(_WIN32) || defined(_WIN64)
    #define SYSMEM_MODE_WINDOWS
#elif defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGE_SIZE)
    #define SYSMEM_MODE_LINUX
#else
    #define SYSMEM_MODE_UNKOWN
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