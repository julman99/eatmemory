#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"

echo "eatmemory backend detection test"
echo "================================"

build_eatmemory

expected_backend="$(detect_expected_backend)"
help_output="$("$EATMEMORY_BIN" -? 2>&1)"
help_first_line="$(printf '%s\n' "$help_output" | sed -n '1p')"
expected_help_suffix=" - https://github.com/julman99/eatmemory - ${expected_backend}"

echo ""
echo "Expected backend: $expected_backend"
echo "Help first line:  $help_first_line"

if [[ "$help_first_line" != *"$expected_help_suffix" ]]; then
    echo "ERROR: expected help output to include backend '$expected_backend'" >&2
    exit 1
fi

echo "Backend detection confirmed"
