#!/bin/bash

# all.test.sh - Run all CI test files ending with .test.sh

set -e  # Exit on any error

echo "Running CI test files..."
echo "======================"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

# Initialize counters
passed=0
failed=0
total_tests=0

echo "Searching for CI test files ending with '.test.sh'..."

while IFS= read -r test_file; do
    # Skip this script itself to prevent infinite recursion
    if [[ "$test_file" == "$SCRIPT_DIR/all.test.sh" ]]; then
        continue
    fi

    total_tests=$((total_tests + 1))

    display_path="${test_file#$PROJECT_ROOT/}"
    echo "Running: $display_path"
    echo "----------------------------------------"

    if bash "$test_file"; then
        echo "✓ PASSED: $display_path"
        passed=$((passed + 1))
    else
        echo "✗ FAILED: $display_path"
        failed=$((failed + 1))
    fi
    echo ""
done < <(find "$SCRIPT_DIR" -type f -name '*.test.sh' | sort)

if [[ "$total_tests" -eq 0 ]]; then
    echo "No CI test files found ending with '.test.sh'"
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
    echo "All CI tests passed!"
    exit 0
else
    echo "Some CI tests failed!"
    exit 1
fi
