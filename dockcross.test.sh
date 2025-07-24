#!/bin/bash
set -e

# Suppress Docker platform warnings on non linux/amd64 platforms
export DOCKER_DEFAULT_PLATFORM=linux/amd64

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

for image in $(cat $SCRIPT_DIR/dockcross.images); do
  echo "************************************************* $image"
  docker run -it --rm -v "$SCRIPT_DIR:/src"  dockcross/$image bash -c 'cd /src && make clean && make'
done