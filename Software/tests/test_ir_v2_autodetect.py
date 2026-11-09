"""Exercise serial protocol selection used by unchanged example connect targets."""

from pathlib import Path
import io
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from ir_upload_v2 import CAPABILITY_BANNER, DEFAULT_COPIES, DEFAULT_FEC_COPIES
from litex_term import LiteXTerm, sfl_magic_ack, sfl_magic_req


class InputPort:
    def __init__(self, term, incoming):
        self.term = term
        self.incoming = iter(incoming)
        self.writes = []

    def read(self):
        try:
            return bytes((next(self.incoming),))
        except StopIteration:
            self.term.reader_alive = False
            return b""

    def write(self, data):
        self.writes.append(data)


class OutputSink:
    def __init__(self):
        self.buffer = io.BytesIO()

    def flush(self):
        pass

    def write(self, data):
        return len(data)


class VersionedSelectionTest(unittest.TestCase):
    def run_stream(self, incoming, region_count=1, explicit=False,
                   fec_enabled=True, copies_override=None, cancel_v2=False):
        # Use the actual reader and SFL handshake, with only serial I/O and
        # flash upload actions replaced by recording stubs.
        term = LiteXTerm.__new__(LiteXTerm)
        term.mem_regions = {"firmware.bin": "0x260000"}
        if region_count == 2:
            term.mem_regions["other.bin"] = "0x240000"
        term.serial_boot = False
        term.ir_v2_requested = explicit
        term.ir_v2 = explicit
        term.ir_fec_enabled = fec_enabled
        term.ir_fec = False
        term.ir_v2_copies_override = copies_override
        term.ir_v2_copies = DEFAULT_COPIES
        term.ir_v2_advertised = False
        term.magic_detect_buffer = bytes(len(sfl_magic_req))
        term.ir_v2_detect_buffer = bytes(len(CAPABILITY_BANNER))
        term.reader_alive = True
        term.port = InputPort(term, incoming)
        choices = []
        term.upload = lambda filename, address: choices.append("legacy")
        def record_v2(filename, address):
            choices.append("v2+fec" if term.ir_fec else "v2")
            return False if cancel_v2 else None

        term.upload_v2 = record_v2
        term.boot = lambda: choices.append("boot") if cancel_v2 else None
        with patch.object(sys, "stdout", OutputSink()):
            term.reader()
        return choices, term.port.writes, term.ir_v2_copies

    def test_remote_advertisement_uses_v2_for_make_connect(self):
        choices, writes, copies = self.run_stream(CAPABILITY_BANNER + sfl_magic_req)
        self.assertEqual(choices, ["v2+fec"])
        self.assertEqual(writes, [sfl_magic_ack])
        self.assertEqual(copies, DEFAULT_FEC_COPIES)

    def test_direct_cable_and_old_remote_stay_legacy(self):
        choices, _, _ = self.run_stream(sfl_magic_req)
        self.assertEqual(choices, ["legacy"])

    def test_advertisement_is_scoped_to_one_request(self):
        choices, _, copies = self.run_stream(CAPABILITY_BANNER + sfl_magic_req + sfl_magic_req)
        self.assertEqual(choices, ["v2+fec", "legacy"])
        self.assertEqual(copies, DEFAULT_COPIES)

    def test_multi_image_request_stays_legacy(self):
        choices, _, _ = self.run_stream(CAPABILITY_BANNER + sfl_magic_req, region_count=2)
        self.assertEqual(choices, ["legacy", "legacy"])

    def test_explicit_selection_remains_available(self):
        choices, _, _ = self.run_stream(sfl_magic_req, explicit=True)
        self.assertEqual(choices, ["v2"])

    def test_remote_fec_can_be_disabled_for_comparison(self):
        choices, _, copies = self.run_stream(CAPABILITY_BANNER + sfl_magic_req,
                                             fec_enabled=False)
        self.assertEqual(choices, ["v2"])
        self.assertEqual(copies, DEFAULT_COPIES)

    def test_copy_override_applies_to_fec_upload(self):
        choices, _, copies = self.run_stream(CAPABILITY_BANNER + sfl_magic_req,
                                             copies_override=2)
        self.assertEqual(choices, ["v2+fec"])
        self.assertEqual(copies, 2)

    def test_cancel_does_not_send_jump_after_abort(self):
        choices, writes, _ = self.run_stream(CAPABILITY_BANNER + sfl_magic_req,
                                             cancel_v2=True)
        self.assertEqual(choices, ["v2+fec"])
        self.assertEqual(writes, [sfl_magic_ack])


if __name__ == "__main__":
    unittest.main()
