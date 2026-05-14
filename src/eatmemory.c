#include "eatmemory.h"
#include "errors.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define TO_KB 1024UL
#define TO_MB (1024UL * TO_KB)
#define TO_GB (1024UL * TO_MB)

#ifndef EATMEMORY_MALLOC
#define EATMEMORY_MALLOC malloc
#define EATMEMORY_MALLOC_IS_DEFAULT
#endif

#ifndef EATMEMORY_FREE
#define EATMEMORY_FREE free
#define EATMEMORY_FREE_IS_DEFAULT
#endif

#ifndef EATMEMORY_MALLOC_IS_DEFAULT
void *EATMEMORY_MALLOC(size_t size);
#endif

#ifndef EATMEMORY_FREE_IS_DEFAULT
void EATMEMORY_FREE(void *ptr);
#endif

size_t string_to_bytes(char * str, eatmemory_error* error) {
    if (str == NULL) {
        *error = EM_ERROR_PARSE_SYNTAX;
        return 0;
    }

    char* start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (*start == '-') {
        *error = EM_ERROR_PARSE_SYNTAX;
        return 0;
    }

    errno = 0;
    char* endptr = NULL;
    uintmax_t parsed = strtoumax(start, &endptr, 10);
    if (endptr == start) {
        *error = EM_ERROR_PARSE_SYNTAX;
        return 0;
    }
    if (errno == ERANGE || parsed > UINT64_MAX) {
        *error = EM_ERROR_PARSE_OVERFLOW;
        return 0;
    }

    while (isspace((unsigned char)*endptr)) {
        endptr++;
    }

    char unit = '\0';
    if (*endptr != '\0') {
        unit = (char)toupper((unsigned char)*endptr);
        endptr++;

        while (isspace((unsigned char)*endptr)) {
            endptr++;
        }
        if (*endptr != '\0') {
            *error = EM_ERROR_PARSE_SYNTAX;
            return 0;
        }
    }

    // Parse and apply unit math in uint64_t, then narrow to size_t at the
    // return. This lets 32-bit platforms compute '50% of 2 GB' (a 100 GB
    // intermediate, 1 GB final) without spuriously rejecting at the
    // multiplication stage just because the intermediate exceeds size_t.
    uint64_t bytes = (uint64_t)parsed;

    if (unit != '\0') {
        // All unit suffixes are syntactic sugar for `bytes * numerator / denominator`.
        // K/M/G are linear multipliers; % is `bytes * memory_stats.free / 100`.
        // Expressed uniformly, the conversion and its overflow check become a
        // single shared code path.
        uint64_t numerator = 0;
        uint64_t denominator = 1;
        if (unit == 'K') {
            numerator = TO_KB;
        } else if (unit == 'M') {
            numerator = TO_MB;
        } else if (unit == 'G') {
            numerator = TO_GB;
        } else if (unit == '%') {
            struct eatmemory_backend backend;
            eatmemory_init(&backend);
            if (!backend.memory_stats_supported) {
                *error = EM_ERROR_PARSE_INVALID_UNIT;
                return 0;
            }

            struct system_memory_stats memory_stats;
            eatmemory_get_system_memory_stats(&memory_stats);
            numerator = memory_stats.free;
            denominator = 100;
        } else {
            *error = EM_ERROR_PARSE_INVALID_UNIT;
            return 0;
        }

        // Catches the uint64_t multiplication itself wrapping (pathological
        // huge inputs like '99999999999999999G'). numerator > 0 covers '%'
        // where the runtime-derived numerator could theoretically be zero.
        if (numerator > 0 && bytes > UINT64_MAX / numerator) {
            *error = EM_ERROR_PARSE_OVERFLOW;
            return 0;
        }
        // Multiply first, then divide -- preserves precision for small values
        // of `bytes` in the '%' case.
        bytes = bytes * numerator / denominator;
    }

    // On 32-bit the unit-applied value may legitimately exceed SIZE_MAX
    // (e.g., 'eatmemory 5G' on i686). Reject before the narrowing cast.
    // Always-false on 64-bit (SIZE_MAX == UINT64_MAX) but harmless.
    if (bytes > SIZE_MAX) {
        *error = EM_ERROR_PARSE_OVERFLOW;
        return 0;
    }
    *error = EM_ERROR_NONE;
    return (size_t)bytes;  // safe: bounded by the SIZE_MAX check above
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

size_t get_chunk_size(size_t total, size_t chunk_size, size_t chunk_index) {
    if (chunk_size == 0) {
        return 0;
    }

    if (chunk_index > total / chunk_size) {
        return 0;
    }

    size_t offset = chunk_index * chunk_size;
    if (offset >= total) {
        return 0;
    }

    return MIN(chunk_size, total - offset);
}

// Byte pattern written to (and verified from) every allocated byte. The
// pattern depends on the runtime `total` argument -- which originates from
// argv and is therefore opaque to the compiler at translation time -- so the
// compiler cannot constant-fold the write loop or the verification loop.
static inline uint8_t eatmemory_pattern(size_t chunk_index, size_t byte_offset, size_t total) {
    return (uint8_t)((chunk_index + byte_offset) ^ total);
}

struct allocation eat(size_t total, size_t chunk_size, eatmemory_error* error) {
    return eat_with_options(total, chunk_size, false, error);
}

struct allocation eat_with_options(size_t total, size_t chunk_size, bool lock_memory, eatmemory_error* error) {
    struct allocation result = { NULL, 0, total, chunk_size, 0 };
    *error = EM_ERROR_NONE;

    // eat() is a public-API function; guard against the caller passing a
    // zero chunk size, which would otherwise trap on the division below.
    if (chunk_size == 0) {
        *error = EM_ERROR_CHUNK_SIZE_ARG_INVALID;
        return result;
    }

    size_t iterations = total/chunk_size;
    if(total % chunk_size > 0) {
        iterations++;
    }
    //Allocate an array to store all the chunks
    if(iterations > SIZE_MAX / sizeof(uint8_t *)) {
        *error = EM_ERROR_CANNOT_ALLOCATE_MEMORY;
        return result;
    }
    uint8_t** allocations = EATMEMORY_MALLOC(sizeof(uint8_t *) * iterations);
    if(allocations == NULL) {
        *error = EM_ERROR_CANNOT_ALLOCATE_MEMORY;
        return result;
    }
    memset(allocations, 0, sizeof(uint8_t *) * iterations);

    // The chunk count is captured in the allocation handle so that digest()
    // cannot disagree with eat() about how many entries to free.
    result.chunks = allocations;
    result.count = iterations;

    // Allocate every chunk and write the runtime-derived pattern into every
    // byte. The pattern is computed per-byte so the compiler cannot collapse
    // the write loop into a memset or constant store.
    for(size_t i=0; i<iterations; i++){
        size_t allocate = get_chunk_size(total, chunk_size, i);
        uint8_t *buffer = EATMEMORY_MALLOC(sizeof(uint8_t) * allocate);
        if(buffer == NULL){
            digest(result);
            *error = EM_ERROR_CANNOT_ALLOCATE_MEMORY;
            result.chunks = NULL;
            result.count = 0;
            return result;
        }
        for(size_t j=0; j<allocate; j++) {
            buffer[j] = eatmemory_pattern(i, j, total);
        }
        allocations[i] = buffer;
    }

    // Verify every byte was actually written and the writes survived
    // optimization. Defense in depth against optimizers eliding the writes:
    //   1. The expected value depends on the runtime `total`; no constant
    //      folding of the write pattern is possible at translation time.
    //   2. Each read goes through a volatile-qualified pointer, so the
    //      compiler is forbidden from replacing the load with a cached copy
    //      of the value it just wrote.
    //   3. Per-byte differences accumulate via bitwise-OR into diff_acc,
    //      which then drives the error-return control flow. The compiler
    //      must perform every read, compute every XOR, and propagate the
    //      result -- it cannot prove diff_acc is zero without executing
    //      the volatile reads.
    //
    // Cost: one extra pass over every byte. Partial sampling would let the
    // optimizer elide writes to bytes we do not inspect; we want full trust.
    uint64_t diff_acc = 0;
    for(size_t i=0; i<iterations; i++) {
        size_t allocate = get_chunk_size(total, chunk_size, i);
        volatile uint8_t* p = (volatile uint8_t*)allocations[i];
        for(size_t j=0; j<allocate; j++) {
            uint8_t got = p[j];
            uint8_t expected = eatmemory_pattern(i, j, total);
            diff_acc |= (uint64_t)(got ^ expected);
        }
    }

    if(diff_acc != 0) {
        digest(result);
        *error = EM_ERROR_MEMORY_VERIFICATION_FAILED;
        result.chunks = NULL;
        result.count = 0;
        return result;
    }

    if (lock_memory) {
        for(size_t i=0; i<iterations; i++) {
            size_t allocate = get_chunk_size(total, chunk_size, i);
            enum eatmemory_lock_result lock_result = eatmemory_lock_region(allocations[i], allocate);
            if (lock_result == EM_LOCK_UNSUPPORTED) {
                digest(result);
                *error = EM_ERROR_MEMORY_LOCK_UNSUPPORTED;
                result.chunks = NULL;
                result.count = 0;
                result.locked_count = 0;
                return result;
            } else if (lock_result != EM_LOCK_OK) {
                digest(result);
                *error = EM_ERROR_CANNOT_LOCK_MEMORY;
                result.chunks = NULL;
                result.count = 0;
                result.locked_count = 0;
                return result;
            }
            result.locked_count++;
        }
    }

    return result;
}

void digest(struct allocation alloc) {
    if(alloc.chunks == NULL) {
        return;
    }
    for(size_t i=0; i < alloc.locked_count; i++) {
        if(alloc.chunks[i] != NULL) {
            size_t allocate = get_chunk_size(alloc.total, alloc.chunk_size, i);
            eatmemory_unlock_region(alloc.chunks[i], allocate);
        }
    }
    for(size_t i=0; i < alloc.count; i++){
        if(alloc.chunks[i] != NULL) {
            EATMEMORY_FREE(alloc.chunks[i]);
        }
    }
    EATMEMORY_FREE(alloc.chunks);
}
