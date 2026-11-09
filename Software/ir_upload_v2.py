"""Versioned IR upload frames carried inside the existing SFL serial link."""

import hashlib
import struct
import zlib


VERSION = 1
# The remote prints this before the normal SFL request so make connect can
# select the versioned transfer without changing example Makefiles.
CAPABILITY_BANNER = b"POGOBOT-IR-V2\n"
CHUNK_SIZE = 64
# Full-image passes separate copies in time without extra robot-side buffers.
DEFAULT_COPIES = 3
DEFAULT_FEC_COPIES = 1
MAX_COPIES = 5
SLOT_SIZE = 0x20000
SLOT_ADDRESSES = (0x240000, 0x260000)
CMD_START = b"\x10"
CMD_DATA = b"\x11"
CMD_END = b"\x12"
CMD_ABORT = b"\x13"
CMD_PARITY = b"\x14"
FEC_FLAG = 1
FEC_DATA_COUNT = 16
FEC_PARITY_COUNT = 4
FEC_INTERLEAVE = 4


def transfer_id_for_image(image):
    """Use a stable image identity so another command can repair missed chunks."""
    return int.from_bytes(hashlib.sha256(image).digest()[:4], "big")


def _gf_multiply(a, b):
    """Multiply in GF(256) with primitive polynomial 0x11d."""
    product = 0
    for _ in range(8):
        if b & 1:
            product ^= a
        a = ((a << 1) ^ (0x11d if a & 0x80 else 0)) & 0xff
        b >>= 1
    return product


def _gf_power(value, exponent):
    result = 1
    while exponent:
        if exponent & 1:
            result = _gf_multiply(result, value)
        value = _gf_multiply(value, value)
        exponent >>= 1
    return result


# Cauchy coefficients make every square submatrix invertible, so any four
# erased symbols among 16 data and four parity symbols are recoverable.
FEC_COEFFICIENTS = tuple(
    tuple(_gf_power(data_index ^ (FEC_DATA_COUNT + row), 254)
          for data_index in range(FEC_DATA_COUNT))
    for row in range(FEC_PARITY_COUNT)
)


def _parity_symbols(group):
    for row in range(FEC_PARITY_COUNT):
        parity = bytearray(CHUNK_SIZE)
        for data_index, chunk in enumerate(group):
            coefficient = FEC_COEFFICIENTS[row][data_index]
            for offset, value in enumerate(chunk):
                parity[offset] ^= _gf_multiply(coefficient, value)
        yield row, bytes(parity)


def image_frames(image, address, transfer_id, fec=False):
    """Yield (command, payload) pairs for one complete user-slot image."""
    if address not in SLOT_ADDRESSES:
        raise ValueError("IR v2 destination must be 0x240000 or 0x260000")
    if not 0 < len(image) <= SLOT_SIZE:
        raise ValueError("IR v2 image must contain 1 to 131072 bytes")
    if not 0 <= transfer_id <= 0xFFFFFFFF:
        raise ValueError("IR v2 transfer ID must fit in 32 bits")

    image_crc = zlib.crc32(image) & 0xFFFFFFFF
    # Version, reserved flags, transfer ID, mapped address, exact length,
    # 64-byte chunk size, and exact-image CRC use network byte order.
    yield CMD_START, struct.pack(">BBIIIHI", VERSION, FEC_FLAG if fec else 0, transfer_id,
                                 address, len(image), CHUNK_SIZE, image_crc)
    chunk_count = (len(image) + CHUNK_SIZE - 1) // CHUNK_SIZE
    stripe_width = FEC_DATA_COUNT * (FEC_INTERLEAVE if fec else 1)
    for stripe_start in range(0, chunk_count, stripe_width):
        groups = []
        for group_start in range(stripe_start,
                                 min(stripe_start + stripe_width, chunk_count),
                                 FEC_DATA_COUNT):
            group = [image[index * CHUNK_SIZE:(index + 1) * CHUNK_SIZE]
                     for index in range(group_start,
                                        min(group_start + FEC_DATA_COUNT, chunk_count))]
            groups.append((group_start, group))
        if fec:
            # Four groups share a stripe: adjacent losses land in different
            # groups, while each group's parity follows all stripe DATA.
            for local_index in range(FEC_DATA_COUNT):
                for group_start, group in groups:
                    if local_index < len(group):
                        yield (CMD_DATA, struct.pack(">IH", transfer_id,
                                                     group_start + local_index)
                               + group[local_index])
            for group_start, group in groups:
                for row, parity in _parity_symbols(group):
                    yield CMD_PARITY, struct.pack(">IHB", transfer_id,
                                                  group_start // FEC_DATA_COUNT, row) + parity
        else:
            for group_start, group in groups:
                for local_index, chunk in enumerate(group):
                    yield (CMD_DATA, struct.pack(">IH", transfer_id,
                                                 group_start + local_index) + chunk)
    yield CMD_END, struct.pack(">I", transfer_id)


def abort_payload(transfer_id):
    return struct.pack(">I", transfer_id)
