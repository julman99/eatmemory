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
static size_t lock_call_count = 0;
static size_t unlock_call_count = 0;
static size_t fail_lock_call = 0;
static size_t malloc_sizes[MAX_ALLOC_CALLS];
static void *malloc_ptrs[MAX_ALLOC_CALLS];
static void *free_ptrs[MAX_ALLOC_CALLS];
static size_t lock_sizes[MAX_ALLOC_CALLS];
static size_t unlock_sizes[MAX_ALLOC_CALLS];
static void *lock_ptrs[MAX_ALLOC_CALLS];
static void *unlock_ptrs[MAX_ALLOC_CALLS];
static size_t progress_call_count = 0;
static enum eatmemory_progress_stage progress_stages[MAX_ALLOC_CALLS];
static size_t progress_completed[MAX_ALLOC_CALLS];
static size_t progress_totals[MAX_ALLOC_CALLS];
static enum eatmemory_stats_result forced_stats_result = EM_STATS_OK;
static enum eatmemory_lock_result forced_lock_result = EM_LOCK_OK;

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

enum eatmemory_stats_result eatmemory_get_system_memory_stats(struct system_memory_stats* stats) {
    stats->total = 1024UL * 1024UL * 1024UL;
    stats->free = 512UL * 1024UL * 1024UL;

    return forced_stats_result;
}

void eatmemory_get_backend_capabilities(struct eatmemory_backend* backend) {
    backend->memory_stats_supported = true;
    backend->memory_lock_supported = true;
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

enum eatmemory_lock_result eatmemory_lock_region(void *ptr, size_t size) {
    size_t call_number = lock_call_count + 1;
    if (call_number > MAX_ALLOC_CALLS) {
        fprintf(stderr, "too many lock calls: %zu\n", call_number);
        exit(1);
    }

    lock_call_count = call_number;
    lock_ptrs[call_number - 1] = ptr;
    lock_sizes[call_number - 1] = size;

    if (fail_lock_call == call_number) {
        return EM_LOCK_FAILED;
    }

    return forced_lock_result;
}

void eatmemory_unlock_region(void *ptr, size_t size) {
    size_t call_number = unlock_call_count + 1;
    if (call_number > MAX_ALLOC_CALLS) {
        fprintf(stderr, "too many unlock calls: %zu\n", call_number);
        exit(1);
    }

    unlock_call_count = call_number;
    unlock_ptrs[call_number - 1] = ptr;
    unlock_sizes[call_number - 1] = size;
}

static void record_progress(enum eatmemory_progress_stage stage, size_t completed, size_t total, void *context) {
    (void)context;

    size_t call_number = progress_call_count + 1;
    if (call_number > MAX_ALLOC_CALLS) {
        fprintf(stderr, "too many progress calls: %zu\n", call_number);
        exit(1);
    }

    progress_call_count = call_number;
    progress_stages[call_number - 1] = stage;
    progress_completed[call_number - 1] = completed;
    progress_totals[call_number - 1] = total;
}

static void reset_allocator(void) {
    malloc_call_count = 0;
    free_call_count = 0;
    fail_malloc_call = 0;
    lock_call_count = 0;
    unlock_call_count = 0;
    fail_lock_call = 0;
    progress_call_count = 0;
    forced_stats_result = EM_STATS_OK;
    forced_lock_result = EM_LOCK_OK;
    memset(malloc_sizes, 0, sizeof(malloc_sizes));
    memset(malloc_ptrs, 0, sizeof(malloc_ptrs));
    memset(free_ptrs, 0, sizeof(free_ptrs));
    memset(lock_sizes, 0, sizeof(lock_sizes));
    memset(unlock_sizes, 0, sizeof(unlock_sizes));
    memset(lock_ptrs, 0, sizeof(lock_ptrs));
    memset(unlock_ptrs, 0, sizeof(unlock_ptrs));
    memset(progress_stages, 0, sizeof(progress_stages));
    memset(progress_completed, 0, sizeof(progress_completed));
    memset(progress_totals, 0, sizeof(progress_totals));
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
        size_t bytes_in_chunk = get_chunk_size(total, chunk_size, i);
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

static void check_locked_chunks_unlocked_once(size_t expected_count) {
    CHECK_SIZE(unlock_call_count, expected_count);
    for (size_t i = 0; i < expected_count; i++) {
        CHECK(unlock_ptrs[i] == lock_ptrs[i]);
        CHECK_SIZE(unlock_sizes[i], lock_sizes[i]);
    }
}

static void check_progress_call(size_t index, enum eatmemory_progress_stage stage, size_t completed, size_t total) {
    CHECK(progress_stages[index] == stage);
    CHECK_SIZE(progress_completed[index], completed);
    CHECK_SIZE(progress_totals[index], total);
}

static void test_chunk_size_helper(void) {
    CHECK_SIZE(get_chunk_size(4096, 1024, 0), 1024);
    CHECK_SIZE(get_chunk_size(4096, 1024, 3), 1024);
    CHECK_SIZE(get_chunk_size(4096, 1024, 4), 0);
    CHECK_SIZE(get_chunk_size(4097, 1024, 0), 1024);
    CHECK_SIZE(get_chunk_size(4097, 1024, 3), 1024);
    CHECK_SIZE(get_chunk_size(4097, 1024, 4), 1);
    CHECK_SIZE(get_chunk_size(4097, 1024, 5), 0);
    CHECK_SIZE(get_chunk_size(4097, 0, 0), 0);
    CHECK_SIZE(get_chunk_size(0, 1024, 0), 0);
}

static void test_percent_size_uses_available_memory(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    size_t bytes = string_to_bytes("50%", &error);

    CHECK(error == EM_ERROR_NONE);
    CHECK_SIZE(bytes, 256UL * 1024UL * 1024UL);
}

static void test_percent_size_rejects_stats_failure(void) {
    reset_allocator();
    forced_stats_result = EM_STATS_FAILED;

    eatmemory_error error = EM_ERROR_NONE;
    size_t bytes = string_to_bytes("50%", &error);

    CHECK(error == EM_ERROR_PARSE_INVALID_UNIT);
    CHECK_SIZE(bytes, 0);
}

static void test_zero_chunk_size(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(100, 0, false, NULL, &error);

    CHECK(error == EM_ERROR_CHUNK_SIZE_ARG_INVALID);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 0);
    CHECK_SIZE(free_call_count, 0);
}

static void test_one_exact_chunk(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(1024, 1024, false, NULL, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 1);
    CHECK_SIZE(malloc_call_count, 2);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 1);
    CHECK_SIZE(malloc_sizes[1], 1024);

    digest(allocation, NULL);
    CHECK_SIZE(free_call_count, 2);
    check_all_successful_allocations_freed();
}

static void test_multiple_exact_chunks(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, false, NULL, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 4);
    CHECK_SIZE(malloc_call_count, 5);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 4);
    for (size_t i = 1; i < malloc_call_count; i++) {
        CHECK_SIZE(malloc_sizes[i], 1024);
    }

    digest(allocation, NULL);
    CHECK_SIZE(free_call_count, 5);
    check_all_successful_allocations_freed();
}

static void test_smaller_final_chunk(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4097, 1024, false, NULL, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 5);
    CHECK_SIZE(malloc_call_count, 6);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 5);
    CHECK_SIZE(malloc_sizes[1], 1024);
    CHECK_SIZE(malloc_sizes[2], 1024);
    CHECK_SIZE(malloc_sizes[3], 1024);
    CHECK_SIZE(malloc_sizes[4], 1024);
    CHECK_SIZE(malloc_sizes[5], 1);

    digest(allocation, NULL);
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
    struct allocation allocation = eat(ONE_MB_BYTES + 1, chunk_size, false, NULL, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 2);
    CHECK_SIZE(malloc_call_count, 3);
    CHECK_SIZE(malloc_sizes[0], sizeof(uint8_t*) * 2);
    CHECK_SIZE(malloc_sizes[1], ONE_MB_BYTES);
    CHECK_SIZE(malloc_sizes[2], 1);

    digest(allocation, NULL);
    CHECK_SIZE(free_call_count, 3);
    check_all_successful_allocations_freed();
}

static void test_pointer_array_allocation_failure(void) {
    reset_allocator();
    fail_malloc_call = 1;

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, false, NULL, &error);

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
    struct allocation allocation = eat(4096, 1024, false, NULL, &error);

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
    struct allocation allocation = eat(4096, 1024, false, NULL, &error);

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
    struct allocation allocation = eat(SIZE_MAX, 1, false, NULL, &error);

    CHECK(error == EM_ERROR_CANNOT_ALLOCATE_MEMORY);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 0);
    CHECK_SIZE(free_call_count, 0);
}

static void test_byte_pattern_written(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(300, 100, false, NULL, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 3);
    check_byte_pattern(allocation, 300, 100);

    digest(allocation, NULL);
    CHECK_SIZE(free_call_count, 4);
    check_all_successful_allocations_freed();
}

static void test_eat_does_not_lock_by_default(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(300, 100, false, NULL, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 3);
    CHECK_SIZE(lock_call_count, 0);
    CHECK_SIZE(allocation.locked_count, 0);

    digest(allocation, NULL);
    CHECK_SIZE(unlock_call_count, 0);
    CHECK_SIZE(free_call_count, 4);
    check_all_successful_allocations_freed();
}

static void test_progress_callback_reports_stages(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct eatmemory_progress progress = { record_progress, NULL };
    struct allocation allocation = eat(300, 100, false, &progress, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 3);
    CHECK_SIZE(progress_call_count, 8);
    check_progress_call(0, EM_PROGRESS_EATING, 0, 300);
    check_progress_call(1, EM_PROGRESS_EATING, 100, 300);
    check_progress_call(2, EM_PROGRESS_EATING, 200, 300);
    check_progress_call(3, EM_PROGRESS_EATING, 300, 300);
    check_progress_call(4, EM_PROGRESS_VERIFYING, 0, 300);
    check_progress_call(5, EM_PROGRESS_VERIFYING, 100, 300);
    check_progress_call(6, EM_PROGRESS_VERIFYING, 200, 300);
    check_progress_call(7, EM_PROGRESS_VERIFYING, 300, 300);

    digest(allocation, &progress);
    CHECK_SIZE(progress_call_count, 12);
    check_progress_call(8, EM_PROGRESS_FREEING, 0, 300);
    check_progress_call(9, EM_PROGRESS_FREEING, 100, 300);
    check_progress_call(10, EM_PROGRESS_FREEING, 200, 300);
    check_progress_call(11, EM_PROGRESS_FREEING, 300, 300);
    CHECK_SIZE(free_call_count, 4);
    check_all_successful_allocations_freed();
}

static void test_lock_memory_success(void) {
    reset_allocator();

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4097, 1024, true, NULL, &error);

    CHECK(error == EM_ERROR_NONE);
    check_successful_eat(allocation, 5);
    CHECK_SIZE(allocation.locked_count, 5);
    CHECK_SIZE(lock_call_count, 5);
    CHECK(lock_ptrs[0] == allocation.chunks[0]);
    CHECK(lock_ptrs[1] == allocation.chunks[1]);
    CHECK(lock_ptrs[2] == allocation.chunks[2]);
    CHECK(lock_ptrs[3] == allocation.chunks[3]);
    CHECK(lock_ptrs[4] == allocation.chunks[4]);
    CHECK_SIZE(lock_sizes[0], 1024);
    CHECK_SIZE(lock_sizes[1], 1024);
    CHECK_SIZE(lock_sizes[2], 1024);
    CHECK_SIZE(lock_sizes[3], 1024);
    CHECK_SIZE(lock_sizes[4], 1);

    digest(allocation, NULL);
    check_locked_chunks_unlocked_once(5);
    CHECK_SIZE(free_call_count, 6);
    check_all_successful_allocations_freed();
}

static void test_lock_memory_failure_cleans_up(void) {
    reset_allocator();
    fail_lock_call = 3;

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, true, NULL, &error);

    CHECK(error == EM_ERROR_CANNOT_LOCK_MEMORY);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 4);
    CHECK_SIZE(lock_call_count, 3);
    check_locked_chunks_unlocked_once(2);
    CHECK_SIZE(free_call_count, 4);
    check_all_successful_allocations_freed();
}

static void test_lock_memory_unsupported_cleans_up(void) {
    reset_allocator();
    forced_lock_result = EM_LOCK_UNSUPPORTED;

    eatmemory_error error = EM_ERROR_NONE;
    struct allocation allocation = eat(4096, 1024, true, NULL, &error);

    CHECK(error == EM_ERROR_MEMORY_LOCK_UNSUPPORTED);
    CHECK(allocation.chunks == NULL);
    CHECK_SIZE(allocation.count, 0);
    CHECK_SIZE(malloc_call_count, 2);
    CHECK_SIZE(lock_call_count, 1);
    CHECK_SIZE(unlock_call_count, 0);
    CHECK_SIZE(free_call_count, 2);
    check_all_successful_allocations_freed();
}

int main(void) {
    test_chunk_size_helper();
    test_percent_size_uses_available_memory();
    test_percent_size_rejects_stats_failure();
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
    test_eat_does_not_lock_by_default();
    test_progress_callback_reports_stages();
    test_lock_memory_success();
    test_lock_memory_failure_cleans_up();
    test_lock_memory_unsupported_cleans_up();

    printf("malloc-hook allocation tests passed\n");
    return 0;
}
