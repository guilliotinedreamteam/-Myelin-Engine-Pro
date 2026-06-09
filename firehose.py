import numpy as np
import time
import json
import os

class NeuralFirehose:
    """
    High-Performance 1,024 Channel Neural Spike Simulator.
    Generates synthetic electrophysiological data with realistic noise and sparse spiking.
    """
    def __init__(self, channels=1024, sampling_rate=20000, bit_depth=10):
        self.channels = channels
        self.sampling_rate = sampling_rate
        self.bit_depth = bit_depth
        self.max_val = (2**bit_depth) - 1
        
        # Neural parameters
        self.spike_rate = 50  # Hz average per channel
        self.noise_floor = 0.05 * self.max_val
        
        print(f"[MYELIN] Firehose Initialized: {channels} ch @ {sampling_rate}Hz")

    def generate_frame(self, duration_ms=5):
        """
        Generates a chunk of data representing 'duration_ms' of neural activity.
        Returns: np.ndarray (channels, samples)
        """
        num_samples = int((duration_ms / 1000) * self.sampling_rate)
        
        # 1. Generate Gaussian Noise (Base)
        data = np.random.normal(self.max_val/2, self.noise_floor/4, (self.channels, num_samples))
        
        # 2. Simulate Sparse Spiking
        # Probability of a spike starting at any given sample
        spike_prob = self.spike_rate / self.sampling_rate
        spike_mask = np.random.random((self.channels, num_samples)) < spike_prob
        
        # Add simple biphasic spike waveforms using vectorized indexing
        ch, sample_idx = np.where(spike_mask[:, :-5])
        if len(ch) > 0:
            data[ch, sample_idx] += self.noise_floor * 2
            data[ch, sample_idx+1] += self.noise_floor * 2
            data[ch, sample_idx+2] += self.noise_floor * 2
            data[ch, sample_idx+3] -= self.noise_floor * 1
            data[ch, sample_idx+4] -= self.noise_floor * 1
        
        # 3. Clip and Quantize to Bit Depth
        return np.clip(data, 0, self.max_val).astype(np.uint16)

    def benchmark(self, iterations=100):
        """Measures simulation latency."""
        print(f"[MYELIN] Starting Performance Benchmark ({iterations} frames)...")
        start = time.perf_counter()
        for _ in range(iterations):
            self.generate_frame(5)
        end = time.perf_counter()
        
        avg_time = (end - start) / iterations
        data_size_kb = (self.channels * (5/1000 * self.sampling_rate) * 2) / 1024
        
        print(f"[MYELIN] Avg Frame Gen Time: {avg_time*1000:.3f} ms (Target < 1.0ms)")
        print(f"[MYELIN] Throughput: {data_size_kb / avg_time:.2f} KB/s")
        return avg_time

if __name__ == "__main__":
    simulator = NeuralFirehose()
    frame = simulator.generate_frame(5)
    print(f"Frame Shape: {frame.shape} (Channels, Samples)")
    simulator.benchmark()
