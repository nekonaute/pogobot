"""Check the 16+4 systematic erasure code and its wire schedule."""

from itertools import combinations
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from ir_upload_v2 import (CHUNK_SIZE, CMD_START, CMD_DATA, CMD_PARITY, CMD_END,
                          FEC_COEFFICIENTS, FEC_DATA_COUNT, FEC_PARITY_COUNT,
                          FEC_FLAG, _gf_multiply, _gf_power, image_frames)


def full_rank(matrix):
    size = len(matrix)
    matrix = [list(row) for row in matrix]
    for column in range(size):
        pivot = next((row for row in range(column, size)
                      if matrix[row][column]), None)
        if pivot is None:
            return False
        matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
        inverse = _gf_power(matrix[column][column], 254)
        for entry in range(column, size):
            matrix[column][entry] = _gf_multiply(matrix[column][entry], inverse)
        for row in range(column + 1, size):
            factor = matrix[row][column]
            for entry in range(column, size):
                matrix[row][entry] ^= _gf_multiply(factor, matrix[column][entry])
    return True


class FecWireTest(unittest.TestCase):
    def test_every_supported_erasure_submatrix_is_invertible(self):
        # Every combination of up to four missing data symbols and surviving
        # parity equations must be solvable, including missing parity packets.
        for count in range(1, FEC_PARITY_COUNT + 1):
            for rows in combinations(range(FEC_PARITY_COUNT), count):
                for columns in combinations(range(FEC_DATA_COUNT), count):
                    self.assertTrue(full_rank([[FEC_COEFFICIENTS[row][column]
                                                for column in columns] for row in rows]))

    def test_group_layout_and_short_tail(self):
        image = bytes((index * 7 + 3) & 0xff for index in range(17 * 64 + 1))
        frames = list(image_frames(image, 0x260000, 0x12345678, fec=True))
        self.assertEqual(frames[0][0], CMD_START)
        self.assertEqual(frames[0][1][1], FEC_FLAG)
        self.assertEqual([command for command, _ in frames].count(CMD_DATA), 18)
        self.assertEqual([command for command, _ in frames].count(CMD_PARITY), 8)
        self.assertEqual(frames[-1][0], CMD_END)
        self.assertEqual([command for command, _ in frames[1:19]],
                         [CMD_DATA] * 18)
        self.assertEqual([command for command, _ in frames[19:27]],
                         [CMD_PARITY] * 8)
        self.assertEqual([struct.unpack(">H", payload[4:6])[0]
                          for command, payload in frames if command == CMD_DATA][:4],
                         [0, 16, 1, 17])
        self.assertEqual([len(payload) for command, payload in frames
                          if command == CMD_PARITY], [71] * 8)
        self.assertEqual([(struct.unpack(">H", payload[4:6])[0], payload[6])
                          for command, payload in frames if command == CMD_PARITY],
                         [(group, row) for group in range(2) for row in range(4)])
        self.assertEqual([len(payload) for command, payload in frames
                          if command == CMD_DATA and
                          struct.unpack(">H", payload[4:6])[0] == 17], [7])

    def test_full_groups_add_exactly_one_quarter_more_packets(self):
        frames = list(image_frames(bytes(60 * 1024), 0x260000, 1, fec=True))
        counts = {command: sum(frame_command == command for frame_command, _ in frames)
                  for command in (CMD_DATA, CMD_PARITY)}
        self.assertEqual(counts, {CMD_DATA: 960, CMD_PARITY: 240})
        self.assertEqual(CHUNK_SIZE, 64)


if __name__ == "__main__":
    unittest.main()
