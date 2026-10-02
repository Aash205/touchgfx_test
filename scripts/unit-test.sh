#!/usr/bin/env bash
# Build and run the unit tests (no board, no HAL). Exits non-zero on any failure.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "$ROOT/Tests" -B "$ROOT/build/UnitTests" -G Ninja
cmake --build "$ROOT/build/UnitTests"
ctest --test-dir "$ROOT/build/UnitTests" --output-on-failure
