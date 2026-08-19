#!/usr/bin/env bash
# Render one frame headlessly and write a PNG for visual verification.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/build/snapshot.png}"
BMP="${OUT%.png}.bmp"
MIN_BRIGHTNESS="${MIN_BRIGHTNESS:-0.02}"

cd "$ROOT"
mkdir -p "$(dirname "$OUT")"

make -s f3dengine
./f3dengine --snapshot "$BMP"
python3 "$ROOT/scripts/bmp_to_png.py" "$BMP" "$OUT"
rm -f "$BMP"

BRIGHTNESS="$(python3 - <<PY
import struct, sys, zlib
from pathlib import Path

path = Path("$OUT")
data = path.read_bytes()
assert data[:8] == b"\\x89PNG\\r\\n\\x1a\\n"
pos = 8
width = height = None
rgba = None
while pos < len(data):
    length = struct.unpack(">I", data[pos:pos+4])[0]
    tag = data[pos+4:pos+8]
    chunk = data[pos+8:pos+8+length]
    pos += 12 + length
    if tag == b"IHDR":
        width, height = struct.unpack(">II", chunk[:8])
    elif tag == b"IDAT":
        raw = zlib.decompress(chunk)
        row_bytes = width * 4
        rgba = bytearray()
        offset = 0
        for _ in range(height):
            offset += 1  # filter byte
            rgba.extend(raw[offset:offset + row_bytes])
            offset += row_bytes
        break

pixels = len(rgba) // 4
total = sum(rgba[i] + rgba[i+1] + rgba[i+2] for i in range(0, len(rgba), 4))
print(total / (pixels * 3.0))
PY
)"

python3 - <<PY
import sys
brightness = float("$BRIGHTNESS")
min_brightness = float("$MIN_BRIGHTNESS")
if brightness < min_brightness:
    print(f"render check failed: average brightness {brightness:.4f} < {min_brightness}", file=sys.stderr)
    sys.exit(1)
print(f"render check passed: average brightness {brightness:.4f}")
PY

echo "Snapshot: $OUT"
