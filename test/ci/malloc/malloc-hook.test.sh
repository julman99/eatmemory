#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"

cd "$PROJECT_ROOT"

mkdir -p output

"${CC:-gcc}" \
    -Wall \
    -Wextra \
    -std=c99 \
    -O2 \
    -g \
    -DEATMEMORY_MALLOC=test_malloc \
    -DEATMEMORY_FREE=test_free \
    -Isrc \
    -o output/eatmemory_malloc_hook.test \
    src/eatmemory.c \
    test/ci/malloc/eatmemory_malloc_hook.test.c

output/eatmemory_malloc_hook.test
