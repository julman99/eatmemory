#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"

readonly ONE_MB_BYTES=1048576
readonly ONE_MB_KB=$((ONE_MB_BYTES / 1024))
readonly UNDER_1MB_KB=$((ONE_MB_KB - 1))
readonly UNDER_1MB_BYTES=$((ONE_MB_BYTES - 1))
readonly OVER_1MB_BYTES=$((ONE_MB_BYTES + 1))
readonly LARGE_TEST_SIZE="100M"

echo "eatmemory CLI allocation test"
echo "============================="

build_eatmemory

echo ""
echo "Allocations around the 1 MB boundary (${ONE_MB_KB} KB = ${ONE_MB_BYTES} bytes):"
echo "=================================================================================================="

run_success "Just under 1 MB (${UNDER_1MB_KB} KB)" \
    "Eating 1023K in chunks of 1K..." \
    "${UNDER_1MB_KB}K" -t 0

run_success "1 byte under 1 MB (${UNDER_1MB_BYTES} bytes)" \
    "Eating 1M in chunks of 1K..." \
    "${UNDER_1MB_BYTES}" -t 0

run_success "Exactly 1 MB (${ONE_MB_KB} KB)" \
    "Eating 1M in chunks of 1M..." \
    "${ONE_MB_KB}K" -t 0

run_success "1 byte over 1 MB (${OVER_1MB_BYTES} bytes)" \
    "Eating 1M in chunks of 1M..." \
    "${OVER_1MB_BYTES}" -t 0

run_success "Well above 1 MB (${LARGE_TEST_SIZE})" \
    "Eating 100M in chunks of 1M..." \
    "${LARGE_TEST_SIZE}" -t 0

run_success "1% of available memory" \
    "Done, sleeping for 0 seconds before exiting..." \
    "1%" -t 0

run_success "Leading zeroes in size argument" \
    "Eating 1K in chunks of 100 bytes..." \
    "001K" -t 0

run_success "Explicit plus in size argument" \
    "Eating 1K in chunks of 100 bytes..." \
    "+1K" -t 0

echo ""
echo "Allocation CLI tests passed"
