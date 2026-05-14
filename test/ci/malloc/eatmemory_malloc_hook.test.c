#include "eatmemory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALLOC_CALLS 64
#define ONE_MB_BYTES 1048576UL

static size_t malloc_call_count = 0;
static size_t free_call_count = 0;
static size_t fail_malloc_call = 0;
static size_t malloc_sizes[MAX_ALLOC_CALLS];
static void *malloc_ptrs[MAX_ALLOC_CALLS];
static void *free_ptrs[MAX_ALLOC_CALLS];

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

#define CHECK_SIZE(actual, expected) do { \
    size_t actual_value = (actual); \
    size_t expected_value = (expected); \
    if (actual_value != expected_value) { \
        fprintf(stderr, "%s:%d: expected %s == %zu, got %zu\n", \
                __FILE__, __LINE__, #actual, expected_value, actual_value); \
        exit(1); \
    } \
} while (0)

void get_system_memory_stats(struct system_memory_stats* stats) {
    stats->supported = true;
    stats->total = 1024UL * 1024UL * 1024UL;
    stats->free = 512UL * 1024UL * 1024UL;
}

void *test_malloc(size_t size) {
    size_t call_number = malloc_call_count + 1;
    if (call_number > MAX_ALLOC_CALLS) {
        fprintf(stderr, "too many malloc calls: %zu\n", call_number);
        exit(1);
    }

    malloc_call_count = call_number;
    malloc_sizes[call_number - 1] = size;

    if (fail_malloc_call == call_number) {
        malloc_ptrs[call_number - 1] = NULL;
        return NULL;
    }

    void *ptr = malloc(size);
    malloc_ptrs[call_number - 1] = ptr;
    return ptr;
}

void test_free(void *ptr) {
    size_t call_number = free_call_count + 1;
    if (call_number > MAX_ALLOC_CALLS) {
        fprintf(stderr, "too many free calls: %zu\n", call_number);
        exit(1);
    }

    free_call_count = call_number;
    free_ptrs[call_number - 1] = ptr;
    free(ptr);
}

static void reset_allocator(void) {
    malloc_call_count = 0;
    free_call_count = 0;
    fail_malloc_call = 0;
    memset(malloc_sizes, 0, sizeof(malloc_sizes));
    memset(malloc_ptrs, 0, sizeof(malloc_ptrs));
    memset(free_ptrs, 0, sizeof(free_ptrs));
}

static size_t min_size(size_t a, size_t b) {
    if (a < b) {
        return a;
    }
    return b;
}

static void check_pointer_freed_once(void *ptr) {
    size_t seen = 0;
    for (size_t i = 0; i < free_call_count; i++) {
        if (free_ptrs[i] == ptr) {
            seen++;
        }
    }
    CHECK_SIZE(seen, 1);
}

static void check_all_successful_allocations_freed(void) {
    for (size_t i = 0; i < malloc_call_count; i++) {
        if (malloc_ptrs[i] != NULL) {
            check_pointer_freed_once(malloc_ptrs[i]);
        }
    }
}

static void check_successful_eat(struct allocation allocation, size_t expected_count) {
    CHECK(allocation.chunks != NULL);
    CHECK_SIZE(allocation.count, expected_count);
    CHECK(allocation.chunks == (uint8_t**)malloc_ptrs[0]);
    for (size_t i = 0; i < expected_count; i++) {
        CHECK(allocation.chunks[i] == (uint8_t*)malloc_ptrs[i + 1]);
    }
}

static void check_byte_pattern(struct allocation allocation, size_t total, size_t chunk_size) {
    size_t verified = 0;
    for (size_t i = 0; i < allocation.count; i++) {
        size_t bytes_in_chunk = min_size(chunk_size, total - verified);
        for (size_t j = 0; j < bytes_in_chunk; j++) {
            uint8_t expected = (uint8_t)((i + j) ^ total);
            if (allocation.chunks[i][j] != expected) {
                fprintf(stderr,
                        "byte pattern mismatch at chunk %zu offset %zu: expected %u, got %u\n",
                        i,
                        j,
                        (unsigned int)expected,
                        (unsigned int)allocation.chunks[i][j]);
                exit(1);
            }
        }
        verified += bytes_in_chunk;
    }
    CHECK_SIZE(verified, total);
}

static void test_zero_chunk_size(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(100, 0, &error);

    CHECK(error == EM_ERROR_CHUNK_SIZE_ARG_INVALID);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 0);
    CHECK_SIZE(free_call_count, 0);
}

static void test_one_exact_chunk(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(1024, 1024, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 1);
    CHECK_SIZE(malloc_call_count, 2);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 1);
    CHECK_SIZE(malloc_sizes[1], 1024);

    digest(allocation);
    CHECK_SIZE(free_call_count, 2);
    check_all_successful_allocations_freed();
}

static void test_multiple_exact_chunks(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 4);
    CHECK_SIZE(malloc_call_count, 5);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 4);
    for (size_t i = 1; i < malloc_call_count; i++) {
        CHECK_SIZE(malloc_sizes[i], 1024);
    }

    digest(allocation);
    CHECK_SIZE(free_call_count, 5);
    check_all_successful_allocations_freed();
}

static void test_smaller_final_chunk(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4097, 1024, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 5);
    CHECK_SIZE(malloc_call_count, 6);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 5);
    CHECK_SIZE(malloc_sizes[1], 1024);
    CHECK_SIZE(malloc_sizes[2], 1024);
    CHECK_SIZE(malloc_sizes[3], 1024);
    CHECK_SIZE(malloc_sizes[4], 1024);
    CHECK_SIZE(malloc_sizes[5], 1);

    digest(allocation);
    CHECK_SIZE(free_call_count, 6);
    check_all_successful_allocations_freed();
}

static void test_auto_chunk_boundary(void) {
    CHECK_SIZE(get_auto_chunk_size(ONE_MB_BYTES - 1), 1024);
    CHECK_SIZE(get_auto_chunk_size(ONE_MB_BYTES), ONE_MB_BYTES);
    CHECK_SIZE(get_auto_chunk_size(ONE_MB_BYTES + 1), ONE_MB_BYTES);

    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    size_t chunk_size = get_auto_chunk_size(ONE_MB_BYTES + 1);
    struct allocation allocation = eat(ONE_MB_BYTES + 1, chunk_size, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 2);
    CHECK_SIZE(malloc_call_count, 3);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 2);
    CHECK_SIZE(malloc_sizes[1], ONE_MB_BYTES);
    CHECK_SIZE(malloc_sizes[2], 1);

    digest(allocation);
    CHECK_SIZE(free_call_count, 3);
    check_all_successful_allocations_freed();
}

static void test_pointer_array_allocation_failure(void) {
    reset_allocator();
    fail_malloc_call = 1;

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, &error);

    CHECK(error == EM_ERROR_CANNOT_ALLOCATE_MEMORY);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 1);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 4);
    CHECK_SIZE(free_call_count, 0);
}

static void test_first_data_chunk_allocation_failure(void) {
    reset_allocator();
    fail_malloc_call = 2;

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, &error);

    CHECK(error == EM_ERROR_CANNOT_ALLOCATE_MEMORY);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 2);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 4);
    CHECK_SIZE(malloc_sizes[1], 1024);
    CHECK_SIZE(free_call_count, 1);
    check_pointer_freed_once(malloc_ptrs[0]);
}

static void test_later_data_chunk_allocation_failure(void) {
    reset_allocator();
    fail_malloc_call = 4;

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, &error);

    CHECK(error == EM_ERROR_CANNOT_ALLOCATE_MEMORY);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 4);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 4);
    CHECK_SIZE(malloc_sizes[1], 1024);
    CHECK_SIZE(malloc_sizes[2], 1024);
    CHECK_SIZE(malloc_sizes[3], 1024);
    CHECK_SIZE(free_call_count, 3);
    check_pointer_freed_once(malloc_ptrs[0]);
    check_pointer_freed_once(malloc_ptrs[1]);
    check_pointer_freed_once(malloc_ptrs[2]);
}

static void test_huge_iteration_guard(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(SIZE_MAX, 1, &error);

    CHECK(error == EM_ERROR_CANNOT_ALLOCATE_MEMORY);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 0);
    CHECK_SIZE(free_call_count, 0);
}

static void test_byte_pattern_written(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(300, 100, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 3);
    check_byte_pattern(allocation, 300, 100);

    digest(allocation);
    CHECK_SIZE(free_call_count, 4);
    check_all_successful_allocations_freed();
}

int main(void) {
    test_zero_chunk_size();
    test_one_exact_chunk();
    test_multiple_exact_chunks();
    test_smaller_final_chunk();
    test_auto_chunk_boundary();
    test_pointer_array_allocation_failure();
    test_first_data_chunk_allocation_failure();
    test_later_data_chunk_allocation_failure();
    test_huge_iteration_guard();
    test_byte_pattern_written();

    printf("malloc-hook allocation tests passed\n");
    return 0;
}
