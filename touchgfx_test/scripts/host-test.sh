#!/usr/bin/env bash
# Build and run the host unit tests (no board, no HAL). Exits non-zero on any failure.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "$ROOT/Tests/Host" -B "$ROOT/build/HostTests" -G Ninja
cmake --build "$ROOT/build/HostTests"
ctest --test-dir "$ROOT/build/HostTests" --output-on-failure
