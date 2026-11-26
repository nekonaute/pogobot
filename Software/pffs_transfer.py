#!/usr/bin/env python3
"""Upload/download PFFS files through the running flash_file UART shell.

Only the command-line serial transport needs pyserial; the protocol client is
transport-independent for tests. No firmware flashing, implicit formatting,
automatic remote deletion, or automatic replacement is performed.
Uploads are raw by default with exact length in the PFFS v4 catalog.
--envelope adds the optional PFT1 length/CRC header for envelope-aware readers.
"""
from __future__ import annotations

import argparse
import binascii
import os
from pathlib import Path
import secrets
import struct
import sys
import tempfile
import time

CHUNK = 48
MAX_BYTES = 5632 * 256
MAX_LINE = 159


class TransferError(Exception):
    """Rejected command, exhausted retries, or failed data validation."""


class BadPacket(Exception):
    """Retry a damaged response without advancing the transfer offset."""


def chunk_crc(token: int, offset: int, payload: bytes) -> int:
    """Protect routing/ordering fields as well as bytes, matching firmware."""
    return binascii.crc32(struct.pack("<II", token, offset) + payload)


def request_signature(command: str) -> int:
    """Ensure READY confirms the requested ID/name/mode, not merely contents."""
    return binascii.crc32(b"".join(word.encode("ascii") + b"\0" for word in command.split()))


class Client:
    """Stop-and-wait client over a pyserial-like read/write transport.

    read(1) must have a finite timeout. Every retried command is idempotent on
    the server; never restart a writer merely because its ACK was lost.
    """

    def __init__(self, transport, timeout=10.0, retries=3, token=None):
        if not 0 < timeout < 30 or retries < 0:
            raise ValueError("require 0 < timeout < 30 and nonnegative retries")
        self.transport = transport
        self.timeout = timeout
        self.retries = retries
        self.token = token if token is not None else (secrets.randbits(32) or 1)
        if not 0 < self.token <= 0xFFFFFFFF:
            raise ValueError("invalid session token")
        self.partial = bytearray()

    def _frame(self, deadline):
        """Ignore prompts/echo/boot messages; bound noisy line accumulation."""
        while time.monotonic() < deadline:
            byte = self.transport.read(1)
            if not byte:
                continue
            if byte == b"\n":
                line = bytes(self.partial)
                self.partial.clear()
                start = line.find(b"PFT1 ")
                if start < 0:
                    continue
                try:
                    fields = line[start:].decode("ascii").split()
                    if len(fields) >= 3 and int(fields[1], 16) == self.token:
                        return fields[2:]
                except (ValueError, UnicodeError):
                    continue
            else:
                self.partial.extend(byte)
                if len(self.partial) > 1024:
                    self.partial.clear()
        return None

    def request(self, command, kind, valid=lambda fields: True):
        """Retry timeouts/bad chunks, ignoring duplicate/stale acknowledgements."""
        wire = (command + "\n").encode("ascii")
        if len(wire) - 1 > MAX_LINE:
            raise TransferError("command exceeds the shell line limit")
        for _ in range(self.retries + 1):
            if self.transport.write(wire) != len(wire):
                raise TransferError("short UART write")
            deadline = time.monotonic() + self.timeout
            while True:
                fields = self._frame(deadline)
                if fields is None:
                    break
                if fields[0] == "ERROR":
                    reason = fields[1] if len(fields) > 1 else "malformed error"
                    if reason == "CRC":
                        break  # Receiver rejected a damaged command; resend.
                    raise TransferError(f"robot rejected transfer: {reason}")
                if fields[0] != kind:
                    continue
                try:
                    if valid(fields):
                        return fields
                except (BadPacket, ValueError, IndexError):
                    break
        raise TransferError(f"no valid {kind} after {self.retries + 1} attempts")

    def abort(self):
        """Best effort only: abort cannot roll back a partially replaced file."""
        try:
            self.transport.write(f"abort {self.token:08x}\n".encode("ascii"))
        except Exception:
            # Includes pyserial's optional transport-specific timeout errors.
            pass

    @staticmethod
    def _name(name):
        if not 1 <= len(name) <= 32 or any(ord(c) < 33 or ord(c) > 126 for c in name):
            raise TransferError("name must contain 1..32 printable ASCII bytes without spaces")

    def upload(self, source, file_id, name, *, raw=True, replace=False, progress=None):
        """Hash then stream a seekable source. Returns original byte count/CRC.

        Raw uploads preserve native records at byte zero; PFFS v4 records the
        exact length. Pass raw=False to add the optional PFT1 payload envelope.
        """
        self._name(name)
        if not 1 <= file_id <= 80:
            raise TransferError("file ID must be between 1 and 80")
        source.seek(0)
        length, crc = 0, 0
        while block := source.read(65536):
            length += len(block)
            if length > MAX_BYTES - (0 if raw else 16):
                raise TransferError("file exceeds the PFFS maximum size")
            crc = binascii.crc32(block, crc)
        source.seek(0)
        mode = "raw-replace" if raw and replace else "raw" if raw else "replace" if replace else "new"
        command = f"put {self.token:08x} {file_id} {name} {length} {crc:08x} {mode}"
        self.request(command, "PUT",
                     lambda f: len(f) == 5 and int(f[1]) == length and int(f[2], 16) == crc and
                     int(f[3]) == CHUNK and int(f[4], 16) == request_signature(command))
        offset, live_crc = 0, 0
        while block := source.read(CHUNK):
            if offset + len(block) > length:
                raise TransferError("source file grew during upload")
            next_offset = offset + len(block)
            self.request(f"putdata {self.token:08x} {offset} {block.hex()} "
                         f"{chunk_crc(self.token, offset, block):08x}", "ACK",
                         lambda f: len(f) == 2 and int(f[1]) == next_offset)
            offset = next_offset
            live_crc = binascii.crc32(block, live_crc)
            if progress:
                progress(offset, length)
        if offset != length or live_crc != crc:
            raise TransferError("source file changed during upload; not committed")
        self.request(f"putend {self.token:08x}", "DONE",
                     lambda f: len(f) == 4 and f[1] == "PUT" and int(f[2]) == length and int(f[3], 16) == crc)
        return length, crc

    def download(self, target, destination, *, raw=False, length=None, progress=None):
        """Write verified bytes to a binary sink; caller owns atomic publication.

        length optionally retains a prefix of a padded raw file; CRC checking
        still covers the COMPLETE remote stream, including discarded padding.
        Returns (saved byte count, CRC32 of the saved bytes).
        """
        self._name(str(target))
        command = f"get {self.token:08x} {target} {'raw' if raw else 'file'}"
        header = self.request(command, "GET",
                              lambda f: len(f) == 5 and 0 <= int(f[1]) <= MAX_BYTES and
                              0 <= int(f[2], 16) <= 0xFFFFFFFF and int(f[3]) == CHUNK and
                              int(f[4], 16) == request_signature(command))
        size, expected = int(header[1]), int(header[2], 16)
        keep = size if length is None else length
        if not 0 <= keep <= size:
            raise TransferError("requested length exceeds the remote stream")
        # Validate the entire remote stream, but report the digest of the bytes
        # actually saved when --length requests only a prefix.
        offset, crc, saved_crc = 0, 0, 0
        while offset < size:
            count = min(CHUNK, size - offset)

            def valid(fields):
                if len(fields) != 4 or int(fields[1]) != offset:
                    return False
                try:
                    block = bytes.fromhex(fields[2])
                except ValueError as exc:
                    raise BadPacket from exc
                if len(block) != count or int(fields[3], 16) != chunk_crc(self.token, offset, block):
                    raise BadPacket
                return True

            frame = self.request(f"getdata {self.token:08x} {offset}", "DATA", valid)
            block = bytes.fromhex(frame[2])
            if offset < keep:
                piece = block[:min(len(block), keep - offset)]
                if destination.write(piece) != len(piece):
                    raise TransferError("short local write")
                saved_crc = binascii.crc32(piece, saved_crc)
            crc = binascii.crc32(block, crc)
            offset += len(block)
            if progress:
                progress(offset, size)
        if crc != expected:
            raise TransferError("download content checksum mismatch")
        self.request(f"getend {self.token:08x} {crc:08x}", "DONE",
                     lambda f: len(f) == 4 and f[1] == "GET" and int(f[2]) == size and int(f[3], 16) == crc)
        return keep, saved_crc


def download_file(client, target, path, *, overwrite=False, **options):
    """Publish only a complete verified download, with no-clobber by default.

    Temporary output is in the destination directory. Hard-link publication
    fails safely if another process creates the target during the transfer.
    --overwrite uses atomic replace instead. Failures remove the temporary file.
    """
    path = Path(path)
    if path.exists() and not overwrite:
        raise TransferError(f"local file exists: {path}; use --overwrite explicitly")
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="wb", dir=path.parent,
                                         prefix=f".{path.name}.", suffix=".part", delete=False) as output:
            temporary = Path(output.name)
            result = client.download(target, output, **options)
            output.flush()
            os.fsync(output.fileno())
        if overwrite:
            os.replace(temporary, path)
        else:
            os.link(temporary, path)
            temporary.unlink()
        temporary = None
        return result
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="UART device, e.g. /dev/ttyUSB0 or COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=10, help="seconds per response (must be <30)")
    parser.add_argument("--retries", type=int, default=3, help="retries after the initial attempt")
    sub = parser.add_subparsers(dest="operation", required=True)
    put = sub.add_parser("upload", help="create a file, or explicitly replace a same-size allocation")
    put.add_argument("source", type=Path)
    put.add_argument("--id", type=int, required=True)
    put.add_argument("--name", help="PFFS name; defaults to source basename; '-' means unnamed")
    put.add_argument("--replace", action="store_true", help="destructive, non-transactional replacement")
    # Upload raw/envelope selection is separate from download --raw, which
    # explicitly exports physical pages instead of decoding headers/logs.
    put_mode = put.add_mutually_exclusive_group()
    put_mode.add_argument("--raw", dest="raw", action="store_true",
                          help="store bytes without a header (default; exact catalog length)")
    put_mode.add_argument("--envelope", dest="raw", action="store_false",
                          help="prepend a 16-byte PFT1 length/CRC header for envelope-aware readers")
    put.set_defaults(raw=True)
    get = sub.add_parser("download", help="download an ordinary file or a log's committed payload")
    get.add_argument("target", help="PFFS ID or name")
    get.add_argument("destination", type=Path)
    get.add_argument("--raw", action="store_true", help="export all allocated pages including headers/padding")
    get.add_argument("--length", type=int, help="retain this many bytes, while validating the full stream")
    get.add_argument("--overwrite", action="store_true", help="explicitly replace an existing local file")
    args = parser.parse_args(argv)
    if not 0 < args.timeout < 30 or args.retries < 0 or args.baud <= 0:
        parser.error("require 0 < timeout < 30, retries >= 0 and baud > 0")
    try:
        import serial  # Host-only optional dependency, never a firmware dependency.
    except ImportError:
        parser.error("install pyserial on the host: python3 -m pip install pyserial")

    def progress(offset, total):
        # stderr leaves stdout available for a script's final result.
        print(f"\r{offset}/{total} bytes", end="", file=sys.stderr, flush=True)

    client = None
    try:
        # The shell must already be running, with no other terminal using UART.
        with serial.Serial(args.port, args.baud, timeout=0.1, write_timeout=args.timeout) as port:
            client = Client(port, args.timeout, args.retries)
            try:
                if args.operation == "upload":
                    with args.source.open("rb") as source:
                        result = client.upload(source, args.id, args.name or args.source.name,
                                               raw=args.raw, replace=args.replace, progress=progress)
                else:
                    result = download_file(client, args.target, args.destination,
                                           raw=args.raw, length=args.length,
                                           overwrite=args.overwrite, progress=progress)
            except BaseException:
                client.abort()
                raise
        print(file=sys.stderr)
        print(f"{args.operation}: {result[0]} bytes, CRC32={result[1]:08x}")
        return 0
    except (TransferError, OSError, ValueError, serial.SerialException) as exc:
        print(f"\nerror: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\ninterrupted; replacement may be incomplete", file=sys.stderr)
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
