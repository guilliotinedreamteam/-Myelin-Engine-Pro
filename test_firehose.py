import unittest
import numpy as np
from firehose import NeuralFirehose

class TestNeuralFirehose(unittest.TestCase):
    def setUp(self):
        self.channels = 10
        self.sampling_rate = 1000
        self.bit_depth = 10
        self.simulator = NeuralFirehose(
            channels=self.channels,
            sampling_rate=self.sampling_rate,
            bit_depth=self.bit_depth
        )
        self.max_val = (2**self.bit_depth) - 1

    def test_generate_frame_shape(self):
        duration_ms = 10
        expected_samples = int((duration_ms / 1000) * self.sampling_rate)

        frame = self.simulator.generate_frame(duration_ms)

        self.assertEqual(frame.shape, (self.channels, expected_samples))

    def test_generate_frame_bounds(self):
        duration_ms = 10
        frame = self.simulator.generate_frame(duration_ms)

        self.assertTrue(np.all(frame >= 0), "Frame contains values less than 0")
        self.assertTrue(np.all(frame <= self.max_val), f"Frame contains values greater than {self.max_val}")

    def test_generate_frame_dtype(self):
        duration_ms = 10
        frame = self.simulator.generate_frame(duration_ms)

        self.assertEqual(frame.dtype, np.uint16)

    def test_generate_empty_frame(self):
        duration_ms = 0
        frame = self.simulator.generate_frame(duration_ms)

        self.assertEqual(frame.shape, (self.channels, 0))

if __name__ == '__main__':
    unittest.main()
