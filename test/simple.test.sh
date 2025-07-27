#!/bin/bash

# simple.test.sh - Simple test that compiles and runs eatmemory with 100M -t 0
# Assumes POSIX system with make and gcc available

set -e  # Exit on any error

echo "Simple eatmemory test - Build and run with 100M -t 0"
echo "===================================================="

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Project root: $PROJECT_ROOT"
echo "Changing to project directory..."
cd "$PROJECT_ROOT"

echo "Cleaning previous build..."
make clean

echo "Building eatmemory..."
make

echo "Checking if eatmemory executable exists..."
if [[ ! -f "output/eatmemory" ]]; then
    echo "ERROR: eatmemory executable not found at output/eatmemory"
    exit 1
fi

echo "Running eatmemory with 100M -t 0..."
echo "Command: ./output/eatmemory 100M -t 0"
echo "--------------------------------------"

# Run eatmemory with the specified arguments
./output/eatmemory 100M -t 0

echo "--------------------------------------"
echo "✓ Test completed successfully!" 