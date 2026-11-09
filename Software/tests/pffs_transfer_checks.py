"""Exercise the real C transfer/writer code through the host Python client.

No serial device, pyserial installation, Pogosim, or physical flash is used.
The bridge has strict one-way NOR programming and fault-injection controls.
"""
import importlib.util
import io
from contextlib import nullcontext, redirect_stderr, redirect_stdout
from pathlib import Path
import queue
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest import mock
from types import SimpleNamespace

BRIDGE = sys.argv.pop(1)
spec = importlib.util.spec_from_file_location("pffs_transfer", Path(__file__).parents[1] / "pffs_transfer.py")
pft = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pft)


class Pipe:
    """Finite-time reads, plus dropped/corrupted reply injection at line level."""
    def __init__(self):
        self.process = subprocess.Popen([BRIDGE], stdin=subprocess.PIPE, stdout=subprocess.PIPE, bufsize=0)
        self.pending = bytearray()
        self.drop = None
        self.damage = False
        self.redirect = False
        self.damage_upload = False
        self.noise = False
        self.lines = queue.Queue()
        # A background reader gives pipe reads a timeout on Windows as well
        # as POSIX; select() on anonymous pipes is not portable to Windows.
        self.reader = threading.Thread(target=self._read_lines, daemon=True)
        self.reader.start()

    def _read_lines(self):
        while True:
            line = self.process.stdout.readline()
            self.lines.put(line)
            if not line:
                return

    def write(self, data):
        if self.redirect and data.startswith(b"put "):
            # A UART error changes the requested ID, not the payload CRC.
            self.redirect = False
            data = data.replace(b" 10 binary ", b" 11 binary ", 1)
        if self.damage_upload and data.startswith(b"putdata "):
            self.damage_upload = False
            fields = data.split()
            fields[3] = (b"00" if fields[3][:2] != b"00" else b"ff") + fields[3][2:]
            data = b" ".join(fields) + b"\n"
        return self.process.stdin.write(data)

    def read(self, size):
        if not self.pending:
            try:
                line = self.lines.get(timeout=0.01)
            except queue.Empty:
                return b""
            if self.drop and self.drop in line:
                self.drop = None
                return b""
            if self.damage and b" DATA " in line:
                fields = line.split()
                fields[-1] = b"00000000"
                line = b" ".join(fields) + b"\n"
                self.damage = False
            if self.noise:
                line = b"boot message\r\nflash[42]> " + line
            self.pending.extend(line)
        result = bytes(self.pending[:size])
        del self.pending[:size]
        return result

    def control(self, command):
        self.write((command + "\n").encode())
        line = bytearray()
        while True:
            byte = self.read(1)
            if not byte:
                if self.process.poll() is not None:
                    raise RuntimeError("bridge exited")
                continue
            line.extend(byte)
            if byte == b"\n":
                if line.startswith(b"TEST "):
                    return line.decode().split()[1:]
                line.clear()

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=5)
        self.reader.join(timeout=5)
        self.process.stdout.close()
        if self.process.returncode:
            raise RuntimeError(f"bridge exited {self.process.returncode}")


class Transfers(unittest.TestCase):
    def setUp(self):
        self.pipe = Pipe()

    def tearDown(self):
        self.pipe.close()

    def client(self):
        return pft.Client(self.pipe, timeout=0.08, retries=2)

    def upload(self, data, *, raw=False, **kwargs):
        # Existing envelope/failure fixtures explicitly retain their layout.
        # Tests below omit the real Client argument to verify the new default.
        return self.client().upload(io.BytesIO(data), 10, "binary", raw=raw, **kwargs)

    def download(self, target="binary", **kwargs):
        output = io.BytesIO()
        self.client().download(target, output, **kwargs)
        return output.getvalue()

    def test_lengths_binary_and_raw_envelope(self):
        # Header/chunk/page boundaries and transfers beyond a 4 KiB erase sector.
        for n in (0, 1, 47, 48, 49, 239, 240, 241, 255, 256, 257, 4097, 12000):
            with self.subTest(length=n):
                self.pipe.control("!format")
                data = bytes((i * 17) % 256 for i in range(n))
                self.upload(data)
                self.assertEqual(self.download(), data)
                raw = self.download(raw=True)
                self.assertEqual(raw[:4], b"PFT1")
                self.assertEqual(raw[16:16+n], data)
                self.assertEqual(len(raw) % 256, 0)
                self.assertTrue(all(b == 255 for b in raw[16+n:]))

    def test_raw_upload_and_prefix_download(self):
        data = b"\x00\xffhello" * 60
        self.upload(data, raw=True)
        for keep in (0, 1, 47, len(data)):
            with self.subTest(keep=keep):
                output = io.BytesIO()
                result = self.client().download("binary", output, length=keep)
                self.assertEqual(output.getvalue(), data[:keep])
                self.assertEqual(result, (keep, pft.binascii.crc32(data[:keep])))
        self.assertEqual(self.download(), data)
        self.assertEqual(len(self.download(raw=True)), 512)

    def test_default_upload_preserves_raw_pages_and_replacement(self):
        for n in (0, 1, 239, 240, 255, 256, 257, 512, 4097):
            with self.subTest(length=n):
                self.pipe.control("!format")
                data = bytes(i % 256 for i in range(n))
                result = self.client().upload(io.BytesIO(data), 10, "binary")
                stored = data + b"\xff" * (max(1, (n + 255) // 256) * 256 - n)
                self.assertEqual(result, (n, pft.binascii.crc32(data)))
                self.assertEqual(self.download(raw=True), stored)
                self.assertEqual(self.download(), data)
        self.pipe.control("!format")
        # A native payload resembling the envelope magic is still left intact.
        data = b"PFT1" + bytes(range(252))
        self.client().upload(io.BytesIO(data), 10, "binary")
        self.assertEqual(self.download(), data)
        replacement = b"PMAG" + b"x" * 252
        self.client().upload(io.BytesIO(replacement), 10, "binary", replace=True)
        self.assertEqual(self.download(raw=True), replacement) # Still one page.
        with self.assertRaisesRegex(pft.TransferError, "SIZE"):
            self.client().upload(io.BytesIO(b"x" * 257), 10, "binary", replace=True)
        self.assertEqual(self.download(), replacement)

    def test_raw_replacement_updates_exact_eof_within_one_page(self):
        # Length may vary without reallocating. Shrinking must erase the old
        # suffix; empty files retain one physical page, but download zero bytes.
        self.client().upload(io.BytesIO(b"x" * 256), 10, "binary")
        for data in (b"short", b"", b"\xff" * 255, b"z" * 256):
            with self.subTest(length=len(data)):
                self.client().upload(io.BytesIO(data), 10, "binary", replace=True)
                self.assertEqual(self.download(), data)
                self.assertEqual(self.download(raw=True), data + b"\xff" * (256 - len(data)))

    def test_cli_default_raw_and_optional_envelope(self):
        # Exercise argument parsing and main() against the real C engine, with
        # a pipe replacing only the serial device. No pyserial is required.
        serial = SimpleNamespace(Serial=mock.Mock(side_effect=lambda *a, **k: nullcontext(self.pipe)),
                                 SerialException=OSError)
        with tempfile.TemporaryDirectory() as directory, mock.patch.dict(sys.modules, {"serial": serial}):
            source = Path(directory) / "record.bin"
            data = b"PMAG" + bytes(range(252))
            source.write_bytes(data)
            for flags in ([], ["--raw"], ["--envelope"]):
                with self.subTest(flags=flags):
                    self.pipe.control("!format")
                    args = ["--port", "test", "upload", str(source), "--id", "10", "--name", "binary"]
                    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                        self.assertEqual(pft.main(args + flags), 0)
                    physical = self.download(raw=True)
                    if flags == ["--envelope"]:
                        self.assertEqual(len(physical), 512)
                        self.assertEqual(physical[:4], b"PFT1")
                        self.assertEqual(physical[16:272], data)
                    else:
                        self.assertEqual(physical, data) # Exactly one page.
                    for download_flags in ([], ["--raw"]):
                        target = Path(directory) / "download.bin"
                        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                            self.assertEqual(pft.main(["--port", "test", "download", "binary",
                                                       str(target), "--overwrite"] + download_flags), 0)
                        self.assertEqual(target.read_bytes(), physical if download_flags else data)
            serial.Serial.reset_mock()
            with redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
                pft.main(args + ["--raw", "--envelope"])
            self.assertEqual(error.exception.code, 2)
            serial.Serial.assert_not_called()

    def test_duplicate_ack_and_finish_are_idempotent(self):
        data = bytes(range(256)) * 3
        self.pipe.drop = b" PUT "
        client = self.client()
        # Drop READY, a data ACK, and final DONE in independent transfers.
        client.upload(io.BytesIO(data), 10, "binary")
        self.assertEqual(self.download(), data)
        self.pipe.control("!format")
        before = int(self.pipe.control("!stats")[0])
        self.pipe.drop = b" ACK 240"  # Exactly after programming page zero.
        self.upload(data)
        self.assertEqual(int(self.pipe.control("!stats")[0]) - before, 5)
        self.assertEqual(self.download(), data)
        self.pipe.control("!format")
        self.pipe.drop = b" DONE PUT "
        self.upload(data)
        self.assertEqual(self.download(), data)

    def test_download_retries_and_last_chunk_cache(self):
        data = bytes(range(256)) * 2
        self.upload(data)
        for marker in (b" GET ", b" DATA ", b" DONE GET "):
            self.pipe.drop = marker
            self.assertEqual(self.download(), data)
        self.pipe.damage = True
        self.assertEqual(self.download(), data)

    def test_begin_fingerprint_and_corrupted_upload_packet(self):
        self.pipe.redirect = True
        client = self.client()
        before = self.pipe.control("!stats")[:2]
        with self.assertRaises(pft.TransferError):
            client.upload(io.BytesIO(b"must not go to ID 11"), 10, "binary")
        self.assertEqual(self.pipe.control("!stats")[:2], before)
        client.request(f"abort {client.token:08x}", "ABORT")
        with self.assertRaisesRegex(pft.TransferError, "NOT_FOUND"):
            self.download("11")
        self.pipe.damage_upload = True
        data = bytes(range(256)) * 2
        self.upload(data)
        self.assertEqual(self.download(), data)

    def test_interrupted_replacement_is_not_rollback(self):
        self.upload(b"A" * 500)
        client = self.client()
        token = client.token
        data = b"B" * 500
        crc = pft.binascii.crc32(data)
        client.request(f"put {token:08x} 10 binary 500 {crc:08x} replace", "PUT")
        for offset in range(0, 240, 48):
            block = data[offset:offset + 48]
            client.request(f"putdata {token:08x} {offset} {block.hex()} "
                           f"{pft.chunk_crc(token, offset, block):08x}", "ACK")
        client.request(f"abort {token:08x}", "ABORT")
        with self.assertRaisesRegex(pft.TransferError, "READ"):
            self.download()

    def test_restart_before_commit_and_argument_bounds(self):
        client = self.client()
        token = client.token
        for command in (f"put {token:08x} 0 binary 0 0 new",
                        f"put {token:08x} 81 binary 0 0 new",
                        f"put {token:08x} 10 binary 4294967296 0 new",
                        f"put {token:08x} 10 binary 1441777 0 new"):
            with self.assertRaises(pft.TransferError):
                client.request(command, "PUT")
        client.request(f"put {token:08x} 10 binary 256 0 new", "PUT")
        for offset in range(0, 240, 48):
            block = b"X" * 48
            client.request(f"putdata {token:08x} {offset} {block.hex()} "
                           f"{pft.chunk_crc(token, offset, block):08x}", "ACK")
        self.pipe.control("!reboot")
        with self.assertRaisesRegex(pft.TransferError, "NOT_FOUND"):
            self.download()
        self.upload(b"new upload after reset")
        self.assertEqual(self.download(), b"new upload after reset")

    def test_packet_limit(self):
        # Largest offset and longest valid name still fit the shell's bound.
        client = self.client()
        packet = f"putdata {client.token:08x} {pft.MAX_BYTES} {'ff' * 48} ffffffff"
        self.assertLessEqual(len(packet), pft.MAX_LINE)
        with self.assertRaisesRegex(pft.TransferError, "line limit"):
            client.request("X" * 160, "ACK")

    def test_console_noise_and_prompt_prefixes(self):
        self.pipe.noise = True
        data = b"bytes following prompts and boot chatter\x00\xff"
        self.upload(data)
        self.assertEqual(self.download(), data)

    def test_replacement_is_explicit_same_allocation_and_name(self):
        self.upload(b"A" * 500)
        before = self.pipe.control("!stats")[:2]
        with self.assertRaisesRegex(pft.TransferError, "CREATE"):
            self.upload(b"B" * 500)
        with self.assertRaisesRegex(pft.TransferError, "SIZE"):
            self.upload(b"B" * 10, replace=True)
        with self.assertRaisesRegex(pft.TransferError, "NAME"):
            self.client().upload(io.BytesIO(b"B" * 500), 10, "other", raw=False, replace=True)
        self.assertEqual(self.pipe.control("!stats")[:2], before)
        self.assertEqual(self.download(), b"A" * 500)
        self.pipe.drop = b" PUT "  # Replacement READY must be retryable too.
        self.upload(b"B" * 600, replace=True)
        self.assertEqual(self.download(), b"B" * 600)

    def test_unformatted_and_corrupt_catalog_never_autoformat(self):
        self.pipe.control("!blank")
        before = self.pipe.control("!stats")[:2]
        with self.assertRaisesRegex(pft.TransferError, "CATALOG"):
            self.upload(b"test")
        self.assertEqual(self.pipe.control("!stats")[:2], before)
        self.pipe.control("!format")
        self.pipe.control("!corruptcat")
        before = self.pipe.control("!stats")[:2]
        with self.assertRaisesRegex(pft.TransferError, "CATALOG"):
            self.upload(b"test")
        self.assertEqual(self.pipe.control("!stats")[:2], before)

    def test_native_files_logs_and_read_integrity(self):
        self.pipe.control("!native")
        self.assertEqual(self.download("native"), bytes(i * 13 % 256 for i in range(512)))
        self.pipe.control("!log")
        before = self.pipe.control("!stats")[:2]
        self.assertEqual(self.download("events"), b"alpha\n\x00\xffx\n")
        self.assertEqual(len(self.download("events", raw=True)), 1024)
        self.assertEqual(self.pipe.control("!stats")[:2], before)
        with self.assertRaisesRegex(pft.TransferError, "FORMAT"):
            self.client().upload(io.BytesIO(b"X" * 1024), 70, "events", raw=True, replace=True)
        self.pipe.control("!corrupt 71")
        with self.assertRaisesRegex(pft.TransferError, "READ"):
            self.download("native")

    def test_short_local_write_and_source_mutation_are_errors(self):
        self.upload(b"test")

        class ShortSink:
            def write(self, block):
                return len(block) - 1

        client = self.client()
        with self.assertRaisesRegex(pft.TransferError, "short local write"):
            client.download("binary", ShortSink())
        client.request(f"abort {client.token:08x}", "ABORT")
        self.pipe.control("!format")
        source = io.BytesIO(b"A" * 100)

        def mutate(offset, total):
            current = source.tell()
            source.seek(50)
            source.write(b"X")
            source.seek(current)

        client = self.client()
        with self.assertRaisesRegex(pft.TransferError, "source file changed"):
            client.upload(source, 10, "binary", progress=mutate)
        client.request(f"abort {client.token:08x}", "ABORT")
        with self.assertRaisesRegex(pft.TransferError, "NOT_FOUND"):
            self.download()

    def test_chunk_validation_order_abort_and_timeout(self):
        client = self.client()
        token = client.token
        client.request(f"put {token:08x} 10 binary 500 00000000 new", "PUT")
        before = self.pipe.control("!stats")[:2]
        with self.assertRaisesRegex(pft.TransferError, "ACK"):
            client.request(f"putdata {token:08x} 0 ff 00000000", "ACK")
        with self.assertRaisesRegex(pft.TransferError, "OFFSET"):
            client.request(f"putdata {token:08x} 1 ff {pft.chunk_crc(token, 1, bytes([255])):08x}", "ACK")
        self.assertEqual(self.pipe.control("!stats")[:2], before)
        with self.assertRaisesRegex(pft.TransferError, "BUSY"):
            self.upload(b"other")
        client.request(f"abort {token:08x}", "ABORT")
        with self.assertRaisesRegex(pft.TransferError, "NOT_FOUND"):
            self.download()
        client = self.client()
        client.request(f"put {client.token:08x} 10 binary 0 00000000 new", "PUT")
        self.pipe.control("!poll 30000")
        self.assertEqual(self.pipe.control("!stats")[2], "0")
        self.upload(b"after timeout")

    def test_whole_checksum_and_torn_write_fail_closed(self):
        client = self.client()
        token = client.token
        client.request(f"put {token:08x} 10 binary 1 00000000 new", "PUT")
        client.request(f"putdata {token:08x} 0 78 {pft.chunk_crc(token, 0, b'x'):08x}", "ACK")
        with self.assertRaisesRegex(pft.TransferError, "CHECKSUM"):
            client.request(f"putend {token:08x}", "DONE")
        with self.assertRaisesRegex(pft.TransferError, "NOT_FOUND"):
            self.download()
        self.pipe.control("!torn")
        with self.assertRaisesRegex(pft.TransferError, "WRITE"):
            self.upload(bytes(range(256)))
        with self.assertRaisesRegex(pft.TransferError, "NOT_FOUND"):
            self.download()

    def test_local_atomic_publication_and_no_clobber(self):
        self.upload(b"download me")
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "file.bin"
            pft.download_file(self.client(), "binary", target)
            self.assertEqual(target.read_bytes(), b"download me")
            with self.assertRaisesRegex(pft.TransferError, "local file exists"):
                pft.download_file(self.client(), "binary", target)
            pft.download_file(self.client(), "binary", target, overwrite=True)
            self.pipe.control("!corrupt 10")
            with self.assertRaises(pft.TransferError):
                pft.download_file(self.client(), "binary", target, overwrite=True)
            self.assertEqual(target.read_bytes(), b"download me")
            self.assertEqual([p.name for p in Path(directory).iterdir()], ["file.bin"])


if __name__ == "__main__":
    unittest.main()
