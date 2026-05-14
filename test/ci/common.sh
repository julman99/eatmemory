#!/bin/bash

CI_TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$CI_TEST_DIR/../.." && pwd)"
EATMEMORY_BIN="$PROJECT_ROOT/output/eatmemory"
CI_BUILD_MARKER="$PROJECT_ROOT/output/.ci-built"

build_eatmemory() {
    echo "Project root: $PROJECT_ROOT"
    cd "$PROJECT_ROOT"

    if [[ ! -f "$CI_BUILD_MARKER" ]]; then
        echo "Cleaning previous build..."
        "${MAKE:-make}" clean
    fi

    echo "Building eatmemory..."
    "${MAKE:-make}"

    if [[ ! -f "$EATMEMORY_BIN" ]]; then
        echo "ERROR: eatmemory executable not found at $EATMEMORY_BIN" >&2
        exit 1
    fi

    mkdir -p "$PROJECT_ROOT/output"
    : > "$CI_BUILD_MARKER"
}

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

assert_output_contains() {
    local output="$1"
    local expected="$2"
    local description="$3"

    if [[ "$output" != *"$expected"* ]]; then
        echo "ERROR: $description" >&2
        echo "Expected output to contain: $expected" >&2
        echo "Actual output:" >&2
        printf '%s\n' "$output" >&2
        exit 1
    fi
}

run_success() {
    local description="$1"
    local expected_output="$2"
    shift 2

    echo ""
    echo "Test: $description"
    echo "Command: $EATMEMORY_BIN $*"
    echo "----------------------------------------------------------------------"

    local output
    local actual_exit
    set +e
    output="$("$EATMEMORY_BIN" "$@" 2>&1)"
    actual_exit=$?
    set -e

    printf '%s\n' "$output"

    if [[ "$actual_exit" -ne 0 ]]; then
        echo "ERROR: expected exit 0, got $actual_exit" >&2
        exit 1
    fi

    if [[ -n "$expected_output" ]]; then
        assert_output_contains "$output" "$expected_output" "$description"
    fi
}

run_failure() {
    local description="$1"
    local expected_exit="$2"
    local expected_output="$3"
    shift 3

    echo ""
    echo "Test: $description"
    echo "Command: $EATMEMORY_BIN $*"
    echo "----------------------------------------------------------------------"

    local output
    local actual_exit
    set +e
    output="$("$EATMEMORY_BIN" "$@" 2>&1)"
    actual_exit=$?
    set -e

    printf '%s\n' "$output"

    if [[ "$actual_exit" -ne "$expected_exit" ]]; then
        echo "ERROR: expected exit $expected_exit, got $actual_exit" >&2
        exit 1
    fi

    assert_output_contains "$output" "$expected_output" "$description"
}
