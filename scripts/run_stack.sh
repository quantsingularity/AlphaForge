#!/usr/bin/env bash
# Build both halves of AlphaForge if needed, then serve the API and the built
# web terminal together on a single port.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${ALPHAFORGE_PORT:-8080}"
BACKEND="$ROOT/code/backend"
FRONTEND="$ROOT/frontend"

echo "[0/3] Checking market data"
if ! ls "$BACKEND"/data/*.csv >/dev/null 2>&1; then
  echo "  No CSV data found. Fetching from Yahoo Finance with yfinance..."
  if command -v python3 >/dev/null 2>&1; then
    pip install -r "$ROOT/requirements.txt" -q || true
    python3 "$ROOT/scripts/fetch_data.py" --period 3y --out "$BACKEND/data" || {
      echo "  Data fetch failed. Run 'python scripts/fetch_data.py' on a network" >&2
      echo "  that can reach Yahoo Finance, then re-run this script." >&2
      exit 1
    }
  else
    echo "  python3 not found. Install Python and run scripts/fetch_data.py first." >&2
    exit 1
  fi
fi

echo "[1/3] Building backend"
cmake -S "$BACKEND" -B "$BACKEND/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$BACKEND/build" -j

echo "[2/3] Building frontend"
if [ ! -d "$FRONTEND/node_modules" ]; then
  (cd "$FRONTEND" && npm install)
fi
(cd "$FRONTEND" && npm run build)

echo "[3/3] Starting AlphaForge on http://localhost:$PORT"
exec "$BACKEND/build/alphaforge_server" "$PORT" "$BACKEND/data" "$FRONTEND/dist"
