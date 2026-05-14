#!/bin/bash

# simple.test.sh - Compiles eatmemory and exercises allocations spanning the
# 1 MB boundary that drives both get_auto_chunk_size() (1 KB chunks below,
# 1 MB chunks at or above) and bytes_to_string() (K vs M formatting), plus
# negative tests covering the parse and chunk-size error paths.

set -e  # Exit on any error

# Sizes straddling the 1 MB unit boundary.
readonly ONE_MB_BYTES=1048576                           # 1 MB in bytes
readonly ONE_MB_KB=$((ONE_MB_BYTES / 1024))             # 1024
readonly UNDER_1MB_KB=$((ONE_MB_KB - 1))                # 1023
readonly UNDER_1MB_BYTES=$((ONE_MB_BYTES - 1))          # 1048575
readonly OVER_1MB_BYTES=$((ONE_MB_BYTES + 1))           # 1048577
readonly LARGE_TEST_SIZE="100M"                         # Comfortably above 1 MB

detect_expected_backend() {
    local uname_s
    uname_s="$(uname -s 2>/dev/null || true)"

    case "$uname_s" in
        Linux*)
            echo "Linux"
            ;;
        Darwin*)
            echo "Darwin"
            ;;
        AIX*)
            echo "AIX"
            ;;
        SunOS*)
            echo "SunOS"
            ;;
        MINGW*|MSYS*|CYGWIN*)
            echo "Windows"
            ;;
        *)
            echo "ERROR: unable to detect expected backend from uname -s: $uname_s" >&2
            return 1
            ;;
    esac
}

echo "eatmemory sanity tests - 1 MB boundary coverage plus error paths"
echo "================================================================="

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Project root: $PROJECT_ROOT"
echo "Changing to project directory..."
cd "$PROJECT_ROOT"

echo "Cleaning previous build..."
"${MAKE:-make}" clean

echo "Building eatmemory..."
"${MAKE:-make}"

echo "Checking if eatmemory executable exists..."
if [[ ! -f "output/eatmemory" ]]; then
    echo "ERROR: eatmemory executable not found at output/eatmemory"
    exit 1
fi

expected_backend="$(detect_expected_backend)"
help_first_line="$(./output/eatmemory -? | sed -n '1p')"
expected_help_suffix=" - https://github.com/julman99/eatmemory - ${expected_backend}"

echo ""
echo "Backend detection check:"
echo "Expected backend: $expected_backend"
echo "Help first line:  $help_first_line"
if [[ "$help_first_line" != *"$expected_help_suffix" ]]; then
    echo "ERROR: expected help output to include backend '$expected_backend'"
    echo "Actual first help line: $help_first_line"
    exit 1
fi
echo "✓ Backend detection confirmed"

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
echo "Allocations around the 1 MB boundary (${ONE_MB_KB} KB = ${ONE_MB_BYTES} bytes):"
echo "=================================================================================================="

# Each test allocates a different size and asserts the binary exits 0,
# implicitly checking that the per-byte readback verification passes.
run_test "1" "Just under 1 MB (${UNDER_1MB_KB} KB)"     "${UNDER_1MB_KB}K -t 0"    "auto chunk size = 1 KB"
run_test "2" "1 byte under 1 MB (${UNDER_1MB_BYTES} bytes)" "${UNDER_1MB_BYTES} -t 0" "auto chunk size = 1 KB"
run_test "3" "Exactly 1 MB (${ONE_MB_KB} KB)"           "${ONE_MB_KB}K -t 0"       "auto chunk size transitions to 1 MB"
run_test "4" "1 byte over 1 MB (${OVER_1MB_BYTES} bytes)"   "${OVER_1MB_BYTES} -t 0"   "auto chunk size = 1 MB, last chunk smaller"
run_test "5" "Well above 1 MB (${LARGE_TEST_SIZE})"     "${LARGE_TEST_SIZE} -t 0"  "auto chunk size = 1 MB, many chunks"

# Percentage allocation: exercises the '%' branch through
# get_system_memory_stats and the bytes * free / 100 conversion. 1% is small
# enough to be safe on every CI runner while still exercising the path. The
# 32-bit-specific 50% regression is covered separately by the
# i686 dockcross run.
run_test "6" "1% of available memory"                  "1% -t 0"                 "% branch with get_system_memory_stats"
run_test "7" "Leading zeroes in size argument"         "001K -t 0"               "numeric parser accepts leading zeroes"
run_test "8" "Explicit plus in size argument"          "+1K -t 0"                "numeric parser accepts an explicit plus sign"

echo ""
echo "Error-path regression tests:"
echo "=================================================================================================="

# Exit codes (see src/errors.h):
#   10 = EM_ERROR_MEMORY_ARG_INVALID       (size parse failed at any stage)
#   11 = EM_ERROR_CHUNK_SIZE_ARG_INVALID   (chunk size invalid, including 0)

# Empty size argument: previously a bounds-violating read of str[len-1] with
# len==0; must now be rejected as a parse error.
run_failing_test "9" "Empty size argument" '""' 10

# Overflow in unit scaling: parsing succeeds but `bytes * TO_GB` would wrap
# size_t. Must be rejected before any allocation is attempted.
run_failing_test "10" "Overflow in G-unit scaling" "99999999999999999G" 10

# Chunk size of zero: previously caused integer division by zero (SIGFPE on
# Linux, SIGBUS on Darwin); must now be rejected cleanly.
run_failing_test "11" "Chunk size of zero" "100M -s 0" 11

# Bare unit suffix: stripping the unit leaves an empty numeric portion;
# sscanf returns EOF and would leave `bytes` uninitialized. The strict
# `== 0` check used to miss this; the fix uses `!= 1` instead.
run_failing_test "12" "Bare unit suffix 'M'" "M" 10
run_failing_test "13" "Bare percent suffix" '"%"' 10
run_failing_test "14" "Negative size argument" "-1M" 10
run_failing_test "15" "Empty timeout value" "1K --timeout=" 1

echo ""
echo "Test 16: Leading whitespace before negative size argument (expect exit 10)"
echo 'Command: ./output/eatmemory " -1M"'
echo "----------------------------------------------------------------------"
set +e
./output/eatmemory " -1M"
actual_exit=$?
set -e
if [[ "$actual_exit" -ne 10 ]]; then
    echo "✗ Test 16 FAILED: expected exit 10, got $actual_exit"
    exit 1
fi
echo "✓ Test 16 completed (exit $actual_exit as expected)"

echo ""
echo "=================================================================================================="
echo "✓ All tests completed successfully"
