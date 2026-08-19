#!/usr/bin/env python3
"""Convert 32-bit BMP (BGRA, bottom-up) to PNG using only the Python stdlib."""

from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path


def _chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def read_bmp_rgba(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    if data[:2] != b"BM":
        raise ValueError(f"{path}: not a BMP file")

    file_size, _reserved1, _reserved2, offset = struct.unpack_from("<IHHI", data, 2)
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40:
        raise ValueError(f"{path}: unsupported DIB header")

    width, height = struct.unpack_from("<ii", data, 18)
    planes, bit_count = struct.unpack_from("<HH", data, 26)
    if planes != 1 or bit_count != 32:
        raise ValueError(f"{path}: expected 32-bit BMP, got {bit_count}-bit")

    if height < 0:
        raise ValueError(f"{path}: top-down BMP not supported")

    row_bytes = width * 4
    padded_row = ((row_bytes + 3) // 4) * 4
    rgba = bytearray(width * height * 4)

    for row in range(height):
        src_y = height - 1 - row
        src_off = offset + src_y * padded_row
        dst_off = row * row_bytes
        for x in range(width):
            b, g, r, a = data[src_off + x * 4 : src_off + x * 4 + 4]
            i = dst_off + x * 4
            rgba[i : i + 4] = bytes((r, g, b, a))

    return width, height, bytes(rgba)


def write_png(path: Path, width: int, height: int, rgba: bytes) -> None:
    # PNG scanlines with filter byte 0 per row.
    raw = bytearray()
    row_bytes = width * 4
    for y in range(height):
        raw.append(0)
        start = y * row_bytes
        raw.extend(rgba[start : start + row_bytes])

    compressed = zlib.compress(bytes(raw), level=9)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)

    png = b"\x89PNG\r\n\x1a\n"
    png += _chunk(b"IHDR", ihdr)
    png += _chunk(b"IDAT", compressed)
    png += _chunk(b"IEND", b"")
    path.write_bytes(png)


def average_brightness(rgba: bytes) -> float:
    if not rgba:
        return 0.0
    total = 0
    pixels = len(rgba) // 4
    for i in range(0, len(rgba), 4):
        total += rgba[i] + rgba[i + 1] + rgba[i + 2]
    return total / (pixels * 3.0)


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <input.bmp> <output.png>", file=sys.stderr)
        return 2

    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    width, height, rgba = read_bmp_rgba(src)
    write_png(dst, width, height, rgba)
    print(f"Wrote {dst} ({width}x{height})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
