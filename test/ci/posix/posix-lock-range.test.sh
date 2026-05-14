#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"

case "$(uname -s 2>/dev/null || true)" in
    MINGW*|MSYS*|CYGWIN*)
        echo "Skipping POSIX lock range tests on Windows"
        exit 0
        ;;
esac

cd "$PROJECT_ROOT"

mkdir -p output

"${CC:-gcc}" \
    -Wall \
    -Wextra \
    -std=c99 \
    -O2 \
    -g \
    ${EXTRA_CFLAGS:-} \
    -Isrc \
    -o output/eatmemory_posix_lock_range.test \
    test/ci/posix/eatmemory_posix_lock_range.test.c

output/eatmemory_posix_lock_range.test
