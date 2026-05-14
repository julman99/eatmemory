#include "eatmemory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_PAGE_SIZE 4096L
#define MAX_LOCK_CALLS 16

static long forced_page_size = TEST_PAGE_SIZE;
static int forced_mlock_result = 0;
static size_t mlock_call_count = 0;
static size_t munlock_call_count = 0;
static void *mlock_ptrs[MAX_LOCK_CALLS];
static void *munlock_ptrs[MAX_LOCK_CALLS];
static size_t mlock_sizes[MAX_LOCK_CALLS];
static size_t munlock_sizes[MAX_LOCK_CALLS];

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

static long test_sysconf(int name) {
    (void)name;
    return forced_page_size;
}

static int test_mlock(const void *ptr, size_t size) {
    size_t call_number = mlock_call_count + 1;
    if (call_number > MAX_LOCK_CALLS) {
        fprintf(stderr, "too many mlock calls: %zu\n", call_number);
        exit(1);
    }

    mlock_call_count = call_number;
    mlock_ptrs[call_number - 1] = (void *)ptr;
    mlock_sizes[call_number - 1] = size;
    return forced_mlock_result;
}

static int test_munlock(const void *ptr, size_t size) {
    size_t call_number = munlock_call_count + 1;
    if (call_number > MAX_LOCK_CALLS) {
        fprintf(stderr, "too many munlock calls: %zu\n", call_number);
        exit(1);
    }

    munlock_call_count = call_number;
    munlock_ptrs[call_number - 1] = (void *)ptr;
    munlock_sizes[call_number - 1] = size;
    return 0;
}

#define EATMEMORY_MLOCK test_mlock
#define EATMEMORY_MUNLOCK test_munlock
#define EATMEMORY_SYSCONF test_sysconf
#include "../../../src/eatmemory.posix-common.c"

static void reset_lock_state(void) {
    forced_page_size = TEST_PAGE_SIZE;
    forced_mlock_result = 0;
    mlock_call_count = 0;
    munlock_call_count = 0;
    for (size_t i = 0; i < MAX_LOCK_CALLS; i++) {
        mlock_ptrs[i] = NULL;
        munlock_ptrs[i] = NULL;
        mlock_sizes[i] = 0;
        munlock_sizes[i] = 0;
    }
}

static void test_unaligned_lock_range_rounds_to_page(void) {
    reset_lock_state();

    enum eatmemory_lock_result result = eatmemory_lock_region((void *)(uintptr_t)0x12345, 100);

    CHECK(result == EM_LOCK_OK);
    CHECK_SIZE(mlock_call_count, 1);
    CHECK(mlock_ptrs[0] == (void *)(uintptr_t)0x12000);
    CHECK_SIZE(mlock_sizes[0], 4096);
}

static void test_cross_page_lock_range_rounds_to_page(void) {
    reset_lock_state();

    enum eatmemory_lock_result result = eatmemory_lock_region((void *)(uintptr_t)0x12ff0, 32);

    CHECK(result == EM_LOCK_OK);
    CHECK_SIZE(mlock_call_count, 1);
    CHECK(mlock_ptrs[0] == (void *)(uintptr_t)0x12000);
    CHECK_SIZE(mlock_sizes[0], 8192);
}

static void test_aligned_lock_range_is_unchanged(void) {
    reset_lock_state();

    enum eatmemory_lock_result result = eatmemory_lock_region((void *)(uintptr_t)0x12000, 4096);

    CHECK(result == EM_LOCK_OK);
    CHECK_SIZE(mlock_call_count, 1);
    CHECK(mlock_ptrs[0] == (void *)(uintptr_t)0x12000);
    CHECK_SIZE(mlock_sizes[0], 4096);
}

static void test_unlock_uses_matching_aligned_range(void) {
    reset_lock_state();

    eatmemory_unlock_region((void *)(uintptr_t)0x12345, 100);

    CHECK_SIZE(munlock_call_count, 1);
    CHECK(munlock_ptrs[0] == (void *)(uintptr_t)0x12000);
    CHECK_SIZE(munlock_sizes[0], 4096);
}

static void test_lock_fails_when_page_size_is_unavailable(void) {
    reset_lock_state();
    forced_page_size = -1;

    enum eatmemory_lock_result result = eatmemory_lock_region((void *)(uintptr_t)0x12345, 100);

    CHECK(result == EM_LOCK_FAILED);
    CHECK_SIZE(mlock_call_count, 0);

    eatmemory_unlock_region((void *)(uintptr_t)0x12345, 100);
    CHECK_SIZE(munlock_call_count, 0);
}

static void test_lock_fails_when_range_overflows(void) {
    reset_lock_state();

    enum eatmemory_lock_result result = eatmemory_lock_region((void *)(UINTPTR_MAX - 7), 16);

    CHECK(result == EM_LOCK_FAILED);
    CHECK_SIZE(mlock_call_count, 0);
}

static void test_zero_length_lock_succeeds_without_mlock(void) {
    reset_lock_state();

    enum eatmemory_lock_result result = eatmemory_lock_region((void *)(uintptr_t)0x12345, 0);

    CHECK(result == EM_LOCK_OK);
    CHECK_SIZE(mlock_call_count, 0);

    eatmemory_unlock_region((void *)(uintptr_t)0x12345, 0);
    CHECK_SIZE(munlock_call_count, 0);
}

static void test_mlock_failure_is_reported(void) {
    reset_lock_state();
    forced_mlock_result = -1;

    enum eatmemory_lock_result result = eatmemory_lock_region((void *)(uintptr_t)0x12345, 100);

    CHECK(result == EM_LOCK_FAILED);
    CHECK_SIZE(mlock_call_count, 1);
}

int main(void) {
    test_unaligned_lock_range_rounds_to_page();
    test_cross_page_lock_range_rounds_to_page();
    test_aligned_lock_range_is_unchanged();
    test_unlock_uses_matching_aligned_range();
    test_lock_fails_when_page_size_is_unavailable();
    test_lock_fails_when_range_overflows();
    test_zero_length_lock_succeeds_without_mlock();
    test_mlock_failure_is_reported();

    printf("POSIX lock range tests passed\n");
    return 0;
}
