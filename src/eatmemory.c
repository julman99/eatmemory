#include "eatmemory.h"
#include "errors.h"
#include <string.h>
#include <ctype.h>
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
                bytes = memory_stats.free * bytes / 100;
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



// Integer round-to-nearest division. Equivalent to round((double)n / d) for
// non-negative inputs, but without the math.h dependency and without an
// intermediate floating-point step that would lose precision near SIZE_MAX.
static size_t div_round_nearest(size_t n, size_t d) {
    return n / d + (n % d >= d / 2 ? 1 : 0);
}

char * bytes_to_string(size_t bytes, char * str){
    if (bytes < 1024) {
        sprintf(str, "%zu bytes", bytes);
    } else if (bytes < 1 * TO_MB -1) {
        sprintf(str, "%zuK", div_round_nearest(bytes, TO_KB));
    } else if (bytes < 1 * TO_GB -1) {
        sprintf(str, "%zuM", div_round_nearest(bytes, TO_MB));
    } else {
        sprintf(str, "%zuG", div_round_nearest(bytes, TO_GB));
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

struct allocation eat(size_t total, size_t chunk, eatmemory_error* error) {
    struct allocation result = { NULL, 0 };
    *error = EM_ERROR_NONE;

    // Get initial memory usage for verification
    struct process_memory_stats initial_memory;
    get_process_memory_stats(&initial_memory);

    size_t iterations = total/chunk;
    if(total % chunk > 0) {
        iterations++;
    }
    //Allocate an array to store all the chunks
    if(iterations > SIZE_MAX / sizeof(uint8_t *)) {
        *error = EM_ERROR_CANNOT_ALLOCATE_MEMORY;
        return result;
    }
    uint8_t** allocations = malloc(sizeof(uint8_t *) * iterations);
    if(allocations == NULL) {
        *error = EM_ERROR_CANNOT_ALLOCATE_MEMORY;
        return result;
    }
    memset(allocations, 0, sizeof(uint8_t *) * iterations);

    // The chunk count is captured in the allocation handle so that digest()
    // cannot disagree with eat() about how many entries to free.
    result.chunks = allocations;
    result.count = iterations;

    //now lets actually allocate each chunk in a way that ensures the memory is written an used
    size_t allocated = 0;
    for(size_t i=0; i<iterations; i++){
        size_t allocate = MIN(chunk, total - allocated);
        uint8_t *buffer = malloc(sizeof(uint8_t) * allocate);
        if(buffer == NULL){
            digest(result);
            *error = EM_ERROR_CANNOT_ALLOCATE_MEMORY;
            result.chunks = NULL;
            result.count = 0;
            return result;
        }
        for(size_t j=0; j<sizeof(uint8_t) * allocate; j++) {
            buffer[j] = 1;
        }
        allocations[i] = buffer;
        allocated += allocate;
    }

    // Verify memory consumption if supported and allocation is large enough
    // For small allocations, OS memory measurement is too imprecise due to:
    // - Page granularity (typically 4KB pages)
    // - Malloc overhead and metadata
    // - Memory alignment requirements
    // - System noise from other processes
    if(initial_memory.supported && total >= MIN_VERIFICATION_THRESHOLD_BYTES) {
        struct process_memory_stats final_memory;
        get_process_memory_stats(&final_memory);

        if(final_memory.supported) {
            // Handle potential underflow if final memory is less than initial
            if(final_memory.rss < initial_memory.rss) {
                // Memory decreased or measurement inconsistency - this is unexpected
                digest(result);
                *error = EM_ERROR_MEMORY_VERIFICATION_FAILED;
                result.chunks = NULL;
                result.count = 0;
                return result;
            }

            size_t memory_increase = final_memory.rss - initial_memory.rss;
            // Allow for some tolerance as there may be additional overhead
            // and other allocations happening in the system
            size_t expected_min, expected_max;

            // Safe calculation of expected_min (80% of total)
            expected_min = (size_t)total * 80 / 100;

            // Safe calculation of expected_max (120% of total) with overflow protection
            if(total > SIZE_MAX / 120) {
                expected_max = SIZE_MAX;
            } else {
                expected_max = total * 120 / 100;
            }

            // Check if memory increase is within expected range
            if(memory_increase < expected_min || memory_increase > expected_max) {
                digest(result);
                *error = EM_ERROR_MEMORY_VERIFICATION_FAILED;
                result.chunks = NULL;
                result.count = 0;
                return result;
            }
        }
    }

    return result;
}

void digest(struct allocation alloc) {
    if(alloc.chunks == NULL) {
        return;
    }
    for(size_t i=0; i < alloc.count; i++){
        if(alloc.chunks[i] != NULL) {
            free(alloc.chunks[i]);
        }
    }
    free(alloc.chunks);
}
