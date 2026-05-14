#!/bin/bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

function echo_test_start {
  echo "***************************************************"
  echo "*********** $1"
  echo "***************************************************"
}

function test_gcc_linux {
  local gcc_version="$1";
  local platform="$2";
  echo_test_start "linux gcc:$gcc_version $2"
  docker run -i -v "$SCRIPT_DIR/../..:/src" --rm --platform $platform gcc:$gcc_version bash -c "cd /src && make clean && make && output/eatmemory 100M -t 0"
}

function test_all_gcc_linux {
  local platforms="linux/amd64 linux/arm64";
  local gcc_versions="4 5 8 9 10 11 12 13";
  for platform in $platforms
  do
    for gcc_version in $gcc_versions
    do
      test_gcc_linux $gcc_version $platform
    done
  done
}

test_all_gcc_linux
