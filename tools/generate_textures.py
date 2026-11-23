#!/usr/bin/env python3
"""Generate small seamless albedo textures with only the Python standard library."""
from pathlib import Path
import math
import random
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "assets"
SIZE = 128

def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)

def write_rgb_png(path: Path, pixels: list[list[tuple[int, int, int]]]) -> None:
    raw = bytearray()
    for row in pixels:
        raw.append(0)
        for red, green, blue in row:
            raw.extend((red, green, blue))
    data = b"\x89PNG\r\n\x1a\n"
    data += png_chunk(b"IHDR", struct.pack(">IIBBBBB", SIZE, SIZE, 8, 2, 0, 0, 0))
    data += png_chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    data += png_chunk(b"IEND", b"")
    path.write_bytes(data)

def periodic_field(seed: int) -> list[list[float]]:
    rng = random.Random(seed)
    scales = (1, 2, 3, 5, 8, 13, 21, 34, 48)
    waves: list[tuple[int, int, float, float]] = []
    for scale in scales:
        for _ in range(5):
            kx = rng.randint(-scale, scale)
            ky = rng.randint(-scale, scale)
            if kx == 0 and ky == 0:
                kx = scale
            waves.append((kx, ky, rng.random() * math.tau, 1.0 / (scale ** 0.72)))
    values = [[0.0] * SIZE for _ in range(SIZE)]
    for y in range(SIZE):
        ay = math.tau * y / SIZE
        for x in range(SIZE):
            ax = math.tau * x / SIZE
            values[y][x] = sum(a * math.sin(kx * ax + ky * ay + phase) for kx, ky, phase, a in waves)
    low = min(min(row) for row in values)
    high = max(max(row) for row in values)
    span = max(high - low, 1e-9)
    return [[((v - low) / span) * 2.0 - 1.0 for v in row] for row in values]

def make_texture(filename: str, base: tuple[int, int, int], strength: float, seed: int) -> None:
    field = periodic_field(seed)
    pixels = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            offset = field[y][x] * strength
            row.append(tuple(max(0, min(255, round(channel + offset))) for channel in base))
        pixels.append(row)
    write_rgb_png(ASSET_DIR / filename, pixels)

if __name__ == "__main__":
    ASSET_DIR.mkdir(parents=True, exist_ok=True)
    make_texture("asphalt_tile.png", (56, 63, 70), 10.0, 91)
    make_texture("grass_tile.png", (111, 151, 93), 12.0, 17)
    make_texture("concrete_tile.png", (164, 166, 158), 7.0, 42)
    make_texture("pavement_tile.png", (190, 178, 151), 9.0, 23)
    print("Generated 128x128 seamless albedo textures in", ASSET_DIR)
