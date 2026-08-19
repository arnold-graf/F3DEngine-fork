#!/usr/bin/env bash
# macOS package power draw while the game runs (requires sudo).
# Example: sudo ./scripts/measure_power.sh 10
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "measure_power.sh is macOS-only (powermetrics)." >&2
  exit 1
fi

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run with sudo: sudo $0 [seconds]" >&2
  exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DURATION="${1:-10}"

cd "$ROOT"
make -s f3dengine

./f3dengine &
PID=$!
trap 'kill "$PID" 2>/dev/null || true' EXIT
sleep 1

echo "Sampling package power for ${DURATION}s while PID $PID runs..."
powermetrics --samplers power -i 1000 -n "$DURATION" 2>/dev/null | awk '
  /Combined Power/ { print }
'
