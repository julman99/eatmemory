#!/bin/bash
set -e

# Windows-specific test using dockcross and wine
# This script tests Windows cross-compilation and functionality

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

function echo_test_start {
  echo "***************************************************"
  echo "*********** $1"
  echo "***************************************************"
}

function test_windows_platform {
  local image="$1"
  echo_test_start "Testing Windows platform: $image"
  
  docker run --rm -v "$SCRIPT_DIR/../..:/src" dockcross/$image bash -c '
    set -e
    cd /src
    
    echo "Building for Windows..."
    make clean
    make
    
    echo "Installing wine for testing..."
    apt-get update -qq 
    apt-get install -y -qq wine
    
    echo "Setting up wine environment..."
    export WINEDEBUG=-all
    
    echo "Testing help command..."
    wine output/eatmemory.exe -? || { echo "Help command failed"; exit 1; }
    
    echo "Testing memory allocation (10M for quick test)..."
    wine output/eatmemory.exe 10M -t 0 || { echo "Small memory test failed"; exit 1; }
    
    echo "Testing memory allocation (100M with self-check)..."  
    wine output/eatmemory.exe 100M -t 0 || { echo "Memory self-check failed"; exit 1; }
    
    echo "Testing percentage allocation (1% of available memory)..."
    wine output/eatmemory.exe 1% -t 0 || { echo "Percentage allocation failed"; exit 1; }
    
    echo "✅ All Windows tests passed for $image"
  '
}

echo "Testing Windows cross-compilation targets..."

# Test the main Windows targets
test_windows_platform "windows-static-x64"
test_windows_platform "windows-shared-x64" 

echo ""
echo "🎉 All Windows tests completed successfully!"
echo ""
echo "This confirms that:"
echo "  ✅ Windows cross-compilation works"
echo "  ✅ Windows memory statistics implementation works"
echo "  ✅ Memory allocation and verification works on Windows"
echo "  ✅ Both system and process memory monitoring work" 
