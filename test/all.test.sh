#!/bin/bash

# all.test.sh - Run all test files ending with .test.sh
# This script discovers and runs all test files in the current directory

set -e  # Exit on any error

echo "Running all test files..."
echo "========================"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Initialize counters
passed=0
failed=0
total_tests=0

echo "Searching for test files ending with '.test.sh'..."

# Use find with proper handling and avoid command injection
while IFS= read -r -d '' test_file; do
    # Skip this script itself
    if [[ "$(basename "$test_file")" == "all.test.sh" ]]; then
        continue
    fi
    
    ((total_tests++))
    
    echo "Running: $test_file"
    echo "----------------------------------------"
    
    # Check if file is executable, if not make it executable
    if [[ ! -x "$test_file" ]]; then
        echo "Making $test_file executable..."
        chmod +x "$test_file"
    fi
    
    # Source the test file in a subshell to provide terminal access
    # while keeping tests isolated from each other and the main script
    if ( source "$test_file" ); then
        echo "✓ PASSED: $test_file"
        ((passed++))
    else
        echo "✗ FAILED: $test_file"
        ((failed++))
    fi
    echo ""
    
done < <(find . -maxdepth 1 -name '*.test.sh' -type f -print0)

if [[ "$total_tests" -eq 0 ]]; then
    echo "No test files found ending with '.test.sh'"
    exit 0
fi

# Summary
echo "========================================"
echo "Test Summary:"
echo "  Total:  $total_tests"
echo "  Passed: $passed"
echo "  Failed: $failed"
echo "========================================"

if [[ "$failed" -eq 0 ]]; then
    echo "All tests passed! 🎉"
    exit 0
else
    echo "Some tests failed! 😞"
    exit 1
fi
