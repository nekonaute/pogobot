#!/usr/bin/env python3
"""Append a deterministic test payload and a CRC-32 seal to a firmware image."""

import argparse
from pathlib import Path
import struct
import subprocess
import zlib


MAX_IMAGE_BYTES = 0x18000  # v3 application ROM window, also checked by main.c


def make_payload(length):
    # Xorshift32 gives reproducible, varying bytes without storing source data.
    state = 0x6D2B79F5
    result = bytearray(length)
    for index in range(length):
        state ^= (state << 13) & 0xFFFFFFFF
        state ^= state >> 17
        state ^= (state << 5) & 0xFFFFFFFF
        result[index] = state >> 24
    return result


def linked_image_bytes(elf, nm):
    # The runtime locates the trailer at _edata_rom, so check the objcopy
    # image really ends there before publishing a bootable-looking binary.
    symbols = {}
    output = subprocess.check_output([nm, str(elf)], text=True)
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2] in ("_ftext", "_edata_rom"):
            symbols[fields[2]] = int(fields[0], 16)
    if len(symbols) != 2 or symbols["_edata_rom"] < symbols["_ftext"]:
        raise ValueError("ELF lacks valid _ftext and _edata_rom bounds")
    return symbols["_edata_rom"] - symbols["_ftext"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("raw_image", type=Path)
    parser.add_argument("sealed_image", type=Path)
    parser.add_argument("payload_bytes", type=int)
    parser.add_argument("elf", type=Path)
    parser.add_argument("nm")
    args = parser.parse_args()

    if args.payload_bytes < 0:
        parser.error("payload_bytes must be nonnegative")
    raw = args.raw_image.read_bytes()
    if len(raw) != linked_image_bytes(args.elf, args.nm):
        parser.error("raw binary length differs from the linked flash image bounds")
    total = len(raw) + 4 + args.payload_bytes + 4
    if total > MAX_IMAGE_BYTES:
        parser.error(f"sealed image ({total} bytes) exceeds {MAX_IMAGE_BYTES}-byte ROM window")

    # The length is immediately after the linked image, so the robot can find
    # the payload and trailing CRC without embedding a build-specific size.
    image = raw + struct.pack("<I", args.payload_bytes) + make_payload(args.payload_bytes)
    checksum = zlib.crc32(image) & 0xFFFFFFFF
    args.sealed_image.write_bytes(image + struct.pack("<I", checksum))
    print(f"Sealed {total} bytes ({args.payload_bytes} payload), CRC32 {checksum:08x}")


if __name__ == "__main__":
    main()
