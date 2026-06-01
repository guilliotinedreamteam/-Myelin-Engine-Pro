import sys
import os
import numpy as np
import time

# Ensure we can import the built extension
sys.path.append(os.path.join(os.getcwd(), "src/telemetry"))
try:
    from myelin_bsde import PyBSDE, PyASS
except ImportError:
    # Look for it in the telemetry folder directly
    sys.path.append("/private/var/root/Myelin-Engine-Pro/src/telemetry")
    from myelin_bsde import PyBSDE, PyASS

from src.simulator.firehose import NeuralFirehose

def test_myelin_compression():
    channels = 1024
    cluster_size = 32
    sampling_rate = 20000
    duration_ms = 5
    
    simulator = NeuralFirehose(channels=channels, sampling_rate=sampling_rate)
    encoder = PyBSDE(channels)
    decoder = PyBSDE(channels)
    squelch = PyASS(channels, cluster_size)
    
    print(f"--- Myelin Engine: Compression Performance Test (v2 with ASS) ---")
    print(f"Config: {channels} channels, {cluster_size} ch/cluster, {sampling_rate}Hz sampling, {duration_ms}ms frames")
    
    # Generate 100 frames
    num_frames = 20
    total_raw_bytes = 0
    total_compressed_bytes = 0
    total_encode_time = 0
    
    samples_per_frame = int((duration_ms / 1000) * sampling_rate)
    
    for i in range(num_frames):
        frame = simulator.generate_frame(duration_ms)
        # Using a flat, C-contiguous numpy array memoryview to pass directly to C++
        flattened_frame = np.ascontiguousarray(frame.T.flatten())
        
        start = time.perf_counter()
        
        # 1. Apply Squelch (ASS) in-place on the memoryview
        squelch.apply(flattened_frame, samples_per_frame)
        
        # 2. Encode (BSDE) directly from the memoryview
        bits = encoder.encode(flattened_frame, samples_per_frame)
        
        total_encode_time += (time.perf_counter() - start)
        
        total_raw_bytes += frame.nbytes
        total_compressed_bytes += len(bits)
        
        # Verify first frame
        if i == 0:
            bits_array = np.frombuffer(bits, dtype=np.uint8)
            decoded_flat = decoder.decode(bits_array, samples_per_frame)
            # Reconstruct needs memoryview
            decoded_array = np.array(decoded_flat, dtype=np.uint16)
            squelch.reconstruct(decoded_array, samples_per_frame)
            reconstructed_frame = decoded_array.reshape(samples_per_frame, channels).T
            if not np.array_equal(frame, reconstructed_frame):
                print(f"[!] Critical Mismatch: Frame 0 failed integrity check!")

                diff_indices = np.where(frame != reconstructed_frame)
                print(f"First mismatch at: {diff_indices[0][0]}, {diff_indices[1][0]}")
                print(f"Expected: {frame[diff_indices[0][0], diff_indices[1][0]]}, Got: {reconstructed_frame[diff_indices[0][0], diff_indices[1][0]]}")
                return False
    
    avg_encode_time_ms = (total_encode_time / num_frames) * 1000
    compression_ratio = total_raw_bytes / total_compressed_bytes
    savings = (1 - (total_compressed_bytes / total_raw_bytes)) * 100
    
    print(f"Results:")
    print(f" - Avg Encode Time: {avg_encode_time_ms:.3f} ms (Target < 1.0ms)")
    print(f" - Raw Data Size: {total_raw_bytes / 1024:.2f} KB")
    print(f" - Compressed Size: {total_compressed_bytes / 1024:.2f} KB")
    print(f" - Compression Ratio: {compression_ratio:.2f}x")
    print(f" - Bandwidth Savings: {savings:.1f}%")
    
    if avg_encode_time_ms < 1.0:
        print(f"Status: PERFECTION ACHIEVED. 😈")
    else:
        print(f"Status: Optimization required. (Current: {avg_encode_time_ms:.3f}ms)")
        
    return True

if __name__ == "__main__":
    test_myelin_compression()
