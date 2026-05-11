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
    // An empty string would make str[len-1] read out of bounds (len-1 wraps
    // to SIZE_MAX for size_t), and a string >= MAX_VALUE_STR_SIZE would not
    // fit in the local buffer with a NUL terminator. Reject both up front.
    if (len == 0 || len >= MAX_VALUE_STR_SIZE) {
        *error = EM_ERROR_PARSE_SYNTAX;
        return 0;
    }

    char unit = str[len - 1];
    char value_numeric[MAX_VALUE_STR_SIZE];

    // memcpy of len+1 explicitly copies the NUL terminator. strncpy would
    // leave value_numeric un-terminated when strlen(str) >= MAX_VALUE_STR_SIZE,
    // which we have ruled out above but the explicit copy is clearer.
    memcpy(value_numeric, str, len + 1);
    if(!isdigit(unit)) {
        value_numeric[len - 1] = '\0';
    }

    //parse bytes into numeric variable
    size_t bytes;
    // Compare against 1, not 0: sscanf returns 0 when matching fails before
    // any conversion, but EOF (-1) when the input is empty. The latter
    // happens after stripping a bare unit suffix (e.g., "M" -> ""), and
    // the strict == 0 check missed it -- letting an uninitialized `bytes`
    // through to the next line.
    if(sscanf(value_numeric, "%zu", &bytes) != 1) {
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

    if(!isdigit(unit)) {
        unit = toupper(unit);
        // All unit suffixes are syntactic sugar for `bytes * numerator / denominator`.
        // K/M/G are linear multipliers; % is `bytes * memory_stats.free / 100`.
        // Expressed uniformly, the conversion and its overflow check become a
        // single shared code path.
        size_t numerator = 0;
        size_t denominator = 1;
        if (unit == 'K') {
            numerator = TO_KB;
        } else if (unit == 'M') {
            numerator = TO_MB;
        } else if (unit == 'G') {
            numerator = TO_GB;
        } else if (unit == '%') {
            struct system_memory_stats memory_stats;
            get_system_memory_stats(&memory_stats);
            if (!memory_stats.supported) {
                *error = EM_ERROR_PARSE_INVALID_UNIT;
                return 0;
            }
            numerator = memory_stats.free;
            denominator = 100;
        } else {
            *error = EM_ERROR_PARSE_INVALID_UNIT;
            return 0;
        }

        // Overflow gate for `bytes * numerator`. The numerator > 0 guard
        // covers '%' where the numerator is runtime-derived and could
        // theoretically be zero (system reports no free memory).
        if (numerator > 0 && bytes > SIZE_MAX / numerator) {
            *error = EM_ERROR_PARSE_OVERFLOW;
            return 0;
        }
        // Multiply first, then divide -- preserves precision for small
        // values of `bytes` in the '%' case.
        bytes = bytes * numerator / denominator;
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

// Byte pattern written to (and verified from) every allocated byte. The
// pattern depends on the runtime `total` argument -- which originates from
// argv and is therefore opaque to the compiler at translation time -- so the
// compiler cannot constant-fold the write loop or the verification loop.
static inline uint8_t eatmemory_pattern(size_t chunk_index, size_t byte_offset, size_t total) {
    return (uint8_t)((chunk_index + byte_offset) ^ total);
}

struct allocation eat(size_t total, size_t chunk_size, eatmemory_error* error) {
    struct allocation result = { NULL, 0 };
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

    // Allocate every chunk and write the runtime-derived pattern into every
    // byte. The pattern is computed per-byte so the compiler cannot collapse
    // the write loop into a memset or constant store.
    size_t allocated = 0;
    for(size_t i=0; i<iterations; i++){
        size_t allocate = MIN(chunk_size, total - allocated);
        uint8_t *buffer = malloc(sizeof(uint8_t) * allocate);
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
        allocated += allocate;
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
    size_t verified = 0;
    for(size_t i=0; i<iterations; i++) {
        size_t allocate = MIN(chunk_size, total - verified);
        volatile uint8_t* p = (volatile uint8_t*)allocations[i];
        for(size_t j=0; j<allocate; j++) {
            uint8_t got = p[j];
            uint8_t expected = eatmemory_pattern(i, j, total);
            diff_acc |= (uint64_t)(got ^ expected);
        }
        verified += allocate;
    }

    if(diff_acc != 0) {
        digest(result);
        *error = EM_ERROR_MEMORY_VERIFICATION_FAILED;
        result.chunks = NULL;
        result.count = 0;
        return result;
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
