#include "eatmemory.h"
#include "errors.h"
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX_VALUE_STR_SIZE 255

#define TO_KB 1024UL
#define TO_MB (1024UL * TO_KB)
#define TO_GB (1024UL * TO_MB)

size_t string_to_bytes(char * str, eatmemory_error* error) {
    const size_t len = strlen(str);
    char unit = str[len - 1];
    char value_numeric[MAX_VALUE_STR_SIZE];

    strncpy(value_numeric, str, MAX_VALUE_STR_SIZE);
    if(!isdigit(unit)) {
        value_numeric[len - 1] = '\0';
    }

    //parse bytes into numeric variable
    size_t bytes;
    if(sscanf(value_numeric, "%zu", &bytes) == 0) {
        *error = EM_ERROR_PARSE_SYNTAX;
        return 0;
    }

    //ensure parsed value can be converted to the original string
    char value_numeric_again[MAX_VALUE_STR_SIZE] = "";
    sprintf(value_numeric_again, "%zu", bytes);
    if(strcmp(value_numeric, value_numeric_again) != 0) {
        *error = EM_ERROR_PARSE_OVERFLOW;
        return 0;
    }

    if(!isdigit(unit) ) {
        unit = toupper(unit);
        if(unit == 'K') {
            bytes = bytes * TO_KB;
        } else if(unit=='M') {
            bytes = bytes * TO_MB;
        } else if(unit=='G') {
            bytes = bytes * TO_GB;
        } else if (unit=='%') {
            struct system_memory_stats memory_stats;
            get_system_memory_stats(&memory_stats);
            if(memory_stats.supported) {
                bytes = bytes * memory_stats.free / 100;
            } else {
                *error = EM_ERROR_PARSE_INVALID_UNIT;
                return 0;
            }
        } else {
            *error = EM_ERROR_PARSE_INVALID_UNIT;
            return 0;
        }
    }

    *error = EM_ERROR_NONE;
    return bytes;
}



char * bytes_to_string(size_t bytes, char * str){
    if (bytes < 1024) {
        sprintf(str, "%zu bytes", bytes);
    } else if (bytes < 1 * TO_MB -1) {
        size_t kb = round(bytes / TO_KB);
        sprintf(str, "%zuK", kb);
    } else if (bytes < 1 * TO_GB -1) {
        size_t mb = round(bytes / TO_MB);
        sprintf(str, "%zuM", mb);
    } else {
        size_t gb = round(bytes / TO_GB);
        sprintf(str, "%zuG", gb);
    }
    return str;
}

size_t get_auto_chunk_size(size_t bytes) {
    if(bytes <= 1024) {
        return 100;
    } else if (bytes < 1 * TO_MB) {
        return 1 * TO_KB;
    } else if (bytes < 1 * TO_GB) {
        return 1 * TO_MB;
    } else {
        return 10 * TO_MB;
    }
}

int8_t** eat(size_t total, size_t chunk, eatmemory_error* error) {
    *error = EM_ERROR_NONE;
    
    // Get initial memory usage for verification
    struct process_memory_stats initial_memory;
    get_process_memory_stats(&initial_memory);
    
    unsigned long iterations = total/chunk;
    if(total % chunk > 0) {
        iterations++;
    }
    //Allocate an array to store all the chunks
    int8_t** allocations = malloc(sizeof(int8_t *) * iterations);
    memset(allocations, 0, sizeof(int8_t *) * iterations);

    //now lets actually allocate each chunk in a way that ensures the memory is written an used
    size_t allocated = 0;
    for(unsigned long i=0; i<iterations; i++){
        size_t allocate = MIN(chunk, total - allocated);
        int8_t *buffer = malloc(sizeof(int8_t) * allocate);
        if(buffer == NULL){
            digest(allocations, total, chunk);
            *error = EM_ERROR_CANNOT_ALLOCATE_MEMORY;
            return NULL;
        }
        for(unsigned long j=0; j<sizeof(int8_t) * allocate; j++) {
            buffer[j] = 1;
        }
        allocations[i] = buffer;
        allocated += allocate;
    }
    
    // Verify memory consumption if supported
    if(initial_memory.supported) {
        struct process_memory_stats final_memory;
        get_process_memory_stats(&final_memory);
        
        if(final_memory.supported) {
            // Handle potential underflow if final memory is less than initial
            if(final_memory.rss < initial_memory.rss) {
                // Memory decreased or measurement inconsistency - this is unexpected
                digest(allocations, total, chunk);
                *error = EM_ERROR_MEMORY_VERIFICATION_FAILED;
                return NULL;
            }
            
            size_t memory_increase = final_memory.rss - initial_memory.rss;
            // Allow for some tolerance as there may be additional overhead
            // and other allocations happening in the system
            size_t expected_min, expected_max;
            
            // Safe calculation of expected_min (80% of total) with overflow protection
            if(total > SIZE_MAX / 80) {
                expected_min = SIZE_MAX; // If overflow would occur, use max value
            } else {
                expected_min = total * 80 / 100;
            }
            
            // Safe calculation of expected_max (120% of total) with overflow protection  
            if(total > SIZE_MAX / 120) {
                expected_max = SIZE_MAX; // If overflow would occur, use max value
            } else {
                expected_max = total * 120 / 100;
            }
            
            // Check if memory increase is within expected range
            if(memory_increase < expected_min || memory_increase > expected_max) {
                digest(allocations, total, chunk);
                *error = EM_ERROR_MEMORY_VERIFICATION_FAILED;
                return NULL;
            }
        }
    }
    
    return allocations;
}

void digest(int8_t** eaten, size_t total, size_t chunk) {
    unsigned long iterations = total/chunk;
    if(total % chunk > 0) {
        iterations++;
    }
    for(unsigned long i=0; i < iterations; i++){
        if(eaten[i] != NULL) {
            free(eaten[i]);
        }
    }
    free(eaten);
}
