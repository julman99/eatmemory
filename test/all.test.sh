#!/bin/bash

# all.test.sh - Run all test files ending with .test.sh
# This script discovers and runs all test files in the current directory

set -e  # Exit on any error

echo "Running all test files..."
echo "========================"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Initialize counters
passed=0
failed=0
total_tests=0

echo "Searching for test files ending with '.test.sh'..."

# Find and run test files using a simple compatible approach
for test_file in test/*.test.sh; do

    # Skip if no files match the pattern
    if [[ ! -f "$test_file" ]]; then
        continue
    fi
    
    # Skip this script itself to prevent infinite recursion
    if [[ "$(basename "$test_file")" == "$(basename "${BASH_SOURCE[0]}")" ]]; then
        continue
    fi
    
    total_tests=$((total_tests + 1))
    
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
        passed=$((passed + 1))
    else
        echo "✗ FAILED: $test_file"
        failed=$((failed + 1))
    fi
    echo ""
    
done

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
