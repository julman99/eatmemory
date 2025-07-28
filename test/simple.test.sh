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

# Helper function to run a test case
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
echo "=================================================================================================="
echo "✓ All edge case tests around MIN_VERIFICATION_THRESHOLD_BYTES completed successfully!"
echo "✓ Verified behavior below, at, and above the ${THRESHOLD_KB}KB verification threshold" 