#!/bin/bash

# simple.test.sh - Simple test that compiles and runs eatmemory with edge cases around MIN_VERIFICATION_THRESHOLD_BYTES
# Tests memory allocations below, at, and above the 1MB verification threshold

set -e  # Exit on any error

# Constants for edge case testing around MIN_VERIFICATION_THRESHOLD_BYTES
readonly THRESHOLD_BYTES=1048576                           # 1MB in bytes
readonly THRESHOLD_KB=$((THRESHOLD_BYTES / 1024))          # 1024KB  
readonly THRESHOLD_MINUS_1_KB=$((THRESHOLD_KB - 1))        # 1023KB
readonly THRESHOLD_MINUS_1_BYTES=$((THRESHOLD_BYTES - 1))  # 1048575 bytes
readonly THRESHOLD_PLUS_1_BYTES=$((THRESHOLD_BYTES + 1))   # 1048577 bytes
readonly LARGE_TEST_SIZE="100M"                            # Well above threshold

echo "Enhanced eatmemory test - Testing edge cases around MIN_VERIFICATION_THRESHOLD_BYTES (${THRESHOLD_KB}KB)"
echo "========================================================================================="

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Project root: $PROJECT_ROOT"
echo "Changing to project directory..."
cd "$PROJECT_ROOT"

echo "Cleaning previous build..."
make clean

echo "Building eatmemory..."
make

echo "Checking if eatmemory executable exists..."
if [[ ! -f "output/eatmemory" ]]; then
    echo "ERROR: eatmemory executable not found at output/eatmemory"
    exit 1
fi

# Helper function to run a test case that should succeed (exit 0).
run_test() {
    local test_num="$1"
    local description="$2"
    local args="$3"
    local expected_behavior="$4"

    echo ""
    echo "Test $test_num: $description - $expected_behavior"
    echo "Command: ./output/eatmemory $args"
    echo "----------------------------------------------------------------------"
    ./output/eatmemory $args
    echo "✓ Test $test_num completed"
}

# Helper for tests that expect a specific non-zero exit code (error paths).
# Asserts the binary exits with exactly $expected_exit and prints a useful
# failure message otherwise.
run_failing_test() {
    local test_num="$1"
    local description="$2"
    local args="$3"
    local expected_exit="$4"

    echo ""
    echo "Test $test_num: $description (expect exit $expected_exit)"
    echo "Command: ./output/eatmemory $args"
    echo "----------------------------------------------------------------------"
    set +e
    ./output/eatmemory $args
    local actual_exit=$?
    set -e
    if [[ "$actual_exit" -ne "$expected_exit" ]]; then
        echo "✗ Test $test_num FAILED: expected exit $expected_exit, got $actual_exit"
        exit 1
    fi
    echo "✓ Test $test_num completed (exit $actual_exit as expected)"
}

echo ""
echo "Testing edge cases around MIN_VERIFICATION_THRESHOLD_BYTES (${THRESHOLD_KB}KB = ${THRESHOLD_BYTES} bytes):"
echo "=================================================================================================="

# Test cases around the verification threshold
run_test "1" "Below threshold (${THRESHOLD_MINUS_1_KB}KB)" "${THRESHOLD_MINUS_1_KB}K -t 0" "Memory verification should be disabled"
run_test "2" "Just below threshold (${THRESHOLD_MINUS_1_BYTES} bytes)" "${THRESHOLD_MINUS_1_BYTES} -t 0" "Memory verification should be disabled"
run_test "3" "Exactly at threshold (${THRESHOLD_KB}KB)" "${THRESHOLD_KB}K -t 0" "Memory verification should be enabled"
run_test "4" "Just above threshold (${THRESHOLD_PLUS_1_BYTES} bytes)" "${THRESHOLD_PLUS_1_BYTES} -t 0" "Memory verification should be enabled"
run_test "5" "Well above threshold (${LARGE_TEST_SIZE})" "${LARGE_TEST_SIZE} -t 0" "Memory verification should be enabled"

echo ""
echo "Error-path regression tests:"
echo "=================================================================================================="

# Exit codes (see src/errors.h):
#   10 = EM_ERROR_MEMORY_ARG_INVALID       (size parse failed at any stage)
#   11 = EM_ERROR_CHUNK_SIZE_ARG_INVALID   (chunk size invalid, including 0)

# Empty size argument: previously a bounds-violating read of str[len-1] with
# len==0; must now be rejected as a parse error.
run_failing_test "6" "Empty size argument" '""' 10

# Overflow in unit scaling: parsing succeeds but `bytes * TO_GB` would wrap
# size_t. Must be rejected before any allocation is attempted.
run_failing_test "7" "Overflow in G-unit scaling" "99999999999999999G" 10

# Chunk size of zero: previously caused integer division by zero (SIGFPE on
# Linux, SIGBUS on Darwin); must now be rejected cleanly.
run_failing_test "8" "Chunk size of zero" "100M -s 0" 11

echo ""
echo "=================================================================================================="
echo "✓ All tests completed successfully"