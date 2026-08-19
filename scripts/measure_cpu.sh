#!/usr/bin/env bash
# Sample process CPU while the game runs (macOS/Linux).
# Wattage requires sudo powermetrics on macOS — see measure_power.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DURATION="${1:-10}"

cd "$ROOT"
make -s f3dengine

./f3dengine &
PID=$!
trap 'kill "$PID" 2>/dev/null || true' EXIT

sleep 1

echo "Sampling CPU for PID $PID for ${DURATION}s..."
if command -v pidpersec >/dev/null 2>&1; then
  pidpersec "$PID" "$DURATION"
elif [[ "$(uname -s)" == "Darwin" ]]; then
  samples=()
  end=$((SECONDS + DURATION))
  while (( SECONDS < end )); do
  if ps -p "$PID" -o %cpu= >/dev/null 2>&1; then
    samples+=("$(ps -p "$PID" -o %cpu= | tr -d ' ')")
  fi
    sleep 0.5
  done
  if ((${#samples[@]} > 0)); then
    printf '%s\n' "${samples[@]}" | awk '{sum+=$1; n++} END {if (n) printf "avg CPU: %.1f%%\n", sum/n}'
  fi
else
  top -b -d 1 -n "$DURATION" -p "$PID" | tail -n +8
fi
