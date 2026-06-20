#!/usr/bin/env bash
# Build the backend and run the unit tests.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND="$ROOT/code/backend"

cmake -S "$BACKEND" -B "$BACKEND/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BACKEND/build" -j
(cd "$BACKEND/build" && ctest --output-on-failure)
