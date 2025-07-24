#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

for image in $(cat $SCRIPT_DIR/dockcross.images); do
  echo "************************************************* $image"
  docker run -it --rm -v "$SCRIPT_DIR:/src"  dockcross/$image bash -c 'cd /src && make clean && make'
done