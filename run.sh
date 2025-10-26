#!/usr/bin/env bash
set -euo pipefail

# Default build type
BUILD_TYPE=Debug

# Parse first argument (r = Release, d = Debug)
if [[ $# -ge 1 ]]; then
  case "$1" in
    r|R) BUILD_TYPE=Release ;;
    d|D) BUILD_TYPE=Debug ;;
    -h|--help)
      echo "Usage: $0 [r|d]"
      echo "  r: Release build"
      echo "  d: Debug build (default)"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [r|d]"
      exit 1
      ;;
  esac
fi

echo ">>> Configuring ($BUILD_TYPE)..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE

echo ">>> Building..."
cmake --build build

EXECUTABLE=./bin/sfmlgame
if [[ ! -x "$EXECUTABLE" ]]; then
  echo "Error: '$EXECUTABLE' not found or not executable"
  exit 2
fi

echo ">>> Running $EXECUTABLE"
exec "$EXECUTABLE"
