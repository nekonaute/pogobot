"""Check the actual terminal frame schedule for repeated IR broadcasts."""

from contextlib import redirect_stdout
from pathlib import Path
import io
import os
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import litex_term
from ir_upload_v2 import (CMD_START, CMD_DATA, CMD_END, CMD_ABORT, CMD_PARITY,
                          DEFAULT_COPIES, transfer_id_for_image)


class RepeatedUploadTest(unittest.TestCase):
    def test_default_needs_no_makefile_option(self):
        with patch.object(litex_term, "Console", lambda: object()), \
             patch.dict(os.environ, {}, clear=True):
            term = litex_term.LiteXTerm(True, "firmware.bin", "0x260000",
                                         None, True, False, 0, False)
        self.assertEqual(term.ir_v2_copies, DEFAULT_COPIES)

    def make_term(self, copies):
        # Construction needs a console only for interactive operation; the
        # upload itself uses the recorded SFL sender below.
        with patch.object(litex_term, "Console", lambda: object()), \
             patch.dict(os.environ, {"POGOBOT_IR_COPIES": str(copies)}):
            term = litex_term.LiteXTerm(True, "firmware.bin", "0x260000",
                                        None, True, False, 0, False)
        term.ir_fec = False  # Direct upload tests bypass the serial announcement.
        return term

    def upload(self, term, image, sender):
        with tempfile.TemporaryDirectory() as directory:
            firmware = Path(directory) / "firmware.bin"
            firmware.write_bytes(image)
            term.send_frame = sender
            with redirect_stdout(io.StringIO()) as output:
                result = term.upload_v2(str(firmware), 0x260000)
            return result, output.getvalue()

    def test_three_passes_repeat_metadata_data_and_completion(self):
        term = self.make_term(3)
        sent = []

        def record(frame):
            sent.append((frame.cmd, frame.payload))
            return True

        length, output = self.upload(term, bytes(range(129)), record)
        self.assertEqual(length, 129)
        expected = [CMD_START, CMD_DATA, CMD_DATA, CMD_DATA, CMD_END, CMD_END]
        for offset in (0, 6, 12):
            self.assertEqual([command for command, _ in sent[offset:offset + 6]], expected)
            self.assertEqual([struct.unpack(">H", payload[4:6])[0]
                              for command, payload in sent[offset:offset + 6]
                              if command == CMD_DATA], [0, 1, 2])
        self.assertEqual(sent[0][1], sent[6][1])
        self.assertEqual(sent[6][1], sent[12][1])
        for position in range(1, 6):
            self.assertEqual(sent[position][1], sent[position + 6][1])
            self.assertEqual(sent[position + 6][1], sent[position + 12][1])
        self.assertEqual(sent[4][1], sent[5][1])
        self.assertIn("Remote acknowledged pass 3/3", output)
        self.assertIn("robot completion unconfirmed", output)
        self.assertIn("Remote pass 1/3", output)
        self.assertIn("Remote pass 3/3", output)
        self.assertEqual(output.count("| 100%"), 3)

    def test_new_command_reuses_image_id_and_changed_image_gets_new_id(self):
        self.assertEqual(transfer_id_for_image(b"abc"), 0xba7816bf)
        term = self.make_term(1)
        sent = []

        def record(frame):
            if frame.cmd == CMD_START:
                sent.append(struct.unpack(">I", frame.payload[2:6])[0])
            return True

        self.upload(term, b"same image", record)
        self.upload(term, b"same image", record)
        self.upload(term, b"new image!", record)
        self.assertEqual(sent[:2], [transfer_id_for_image(b"same image")] * 2)
        self.assertNotEqual(sent[1], sent[2])

    def test_one_pass_and_failure_abort(self):
        term = self.make_term(1)
        sent = []

        def record(frame):
            sent.append(frame.cmd)
            return len(sent) != 2

        with self.assertRaises(IOError):
            self.upload(term, bytes(range(65)), record)
        self.assertEqual(sent, [CMD_START, CMD_DATA, CMD_ABORT])

    def test_parity_is_sent_in_every_pass(self):
        term = self.make_term(2)
        term.ir_fec = True
        sent = []

        def record(frame):
            sent.append((frame.cmd, frame.payload))
            return True

        self.upload(term, bytes(range(256)) * 4 + b"x", record)
        commands = [command for command, _ in sent]
        self.assertEqual(commands.count(CMD_PARITY), 16)
        self.assertEqual(commands.count(CMD_START), 2)
        self.assertEqual(commands.count(CMD_END), 4)
        self.assertTrue(all(payload[1] == 1 for command, payload in sent
                            if command == CMD_START))

    def test_copy_count_is_bounded(self):
        for invalid in ("0", "6", "two"):
            with self.assertRaisesRegex(ValueError, "POGOBOT_IR_COPIES"):
                self.make_term(invalid)

    def test_q_cancels_between_frames_with_abort(self):
        term = self.make_term(1)
        sent = []

        def request_cancel_after_start(frame):
            sent.append(frame.cmd)
            if frame.cmd == CMD_START:
                term.ir_v2_cancel_requested = True
            return True

        result, output = self.upload(term, b"x" * 65, request_cancel_after_start)
        self.assertIs(result, False)
        self.assertEqual(sent, [CMD_START, CMD_ABORT])
        self.assertIn("Press Q to cancel", output)
        self.assertIn("upload cancelled", output)
        self.assertFalse(term.ir_v2_uploading)

        class QConsole:
            def getkey(self):
                term.writer_alive = False
                return b"Q"

        class RecordingPort:
            writes = []

            def write(self, data):
                self.writes.append(data)

        term.console = QConsole()
        term.port = RecordingPort()
        term.writer_alive = True
        term.ir_v2_uploading = True
        term.ir_v2_cancel_requested = False
        term.writer()
        self.assertTrue(term.ir_v2_cancel_requested)
        self.assertEqual(term.port.writes, [])


if __name__ == "__main__":
    unittest.main()
