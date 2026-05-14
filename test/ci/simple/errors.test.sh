#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"

echo "eatmemory CLI error-path test"
echo "============================="

build_eatmemory

run_failure "Empty size argument" \
    10 \
    "ERROR 10: Memory to eat is invalid" \
    ""

run_failure "Overflow in G-unit scaling" \
    10 \
    "ERROR 10: Memory to eat is invalid" \
    "99999999999999999G"

run_failure "Chunk size of zero" \
    11 \
    "ERROR 11: Chunk size must be greater than zero" \
    "100M" -s 0

run_failure "Bare unit suffix 'M'" \
    10 \
    "ERROR 10: Memory to eat is invalid" \
    "M"

run_failure "Bare percent suffix" \
    10 \
    "ERROR 10: Memory to eat is invalid" \
    "%"

run_failure "Negative size argument" \
    10 \
    "ERROR 10: Memory to eat is invalid" \
    "-1M"

run_failure "Empty timeout value" \
    1 \
    "error: missing argument for --timeout" \
    "1K" "--timeout="

run_failure "Leading whitespace before negative size argument" \
    10 \
    "ERROR 10: Memory to eat is invalid" \
    " -1M"

echo ""
echo "Error-path CLI tests passed"
