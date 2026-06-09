#include "bsde.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>

namespace myelin {

class BSDE_New : public BSDE {
public:
    BSDE_New(int channels) : BSDE(channels) {}

    std::vector<uint16_t> decode(const uint8_t* bits, int bits_len, int samples) {
        std::vector<uint16_t> frame(m_channels * samples);
        int bit_idx = 0;
        int byte_idx = 0;

        auto get_bit = [&]() -> bool {
            if (byte_idx >= bits_len) return false;
            bool bit = (bits[byte_idx] >> (7 - bit_idx)) & 1;
            bit_idx++;
            if (bit_idx > 7) {
                bit_idx = 0;
                byte_idx++;
            }
            return bit;
        };

        auto get_bits = [&](int count) -> uint32_t {
            uint32_t val = 0;
            while (count > 0) {
                if (byte_idx >= bits_len) return val << count;

                int bits_available = 8 - bit_idx;
                int bits_to_read = std::min(count, bits_available);

                uint32_t extracted_bits = (bits[byte_idx] >> (bits_available - bits_to_read)) & ((1 << bits_to_read) - 1);

                val = (val << bits_to_read) | extracted_bits;

                bit_idx += bits_to_read;
                if (bit_idx >= 8) {
                    bit_idx = 0;
                    byte_idx++;
                }

                count -= bits_to_read;
            }
            return val;
        };

        for (int s = 0; samples > s; ++s) {
            for (int c = 0; c < m_channels; ++c) {
                if (!get_bit()) {
                    // delta is 0
                    frame[s * m_channels + c] = m_prev_values[c];
                } else {
                    if (!get_bit()) {
                        // small delta
                        uint8_t encoded_delta = get_bits(4);
                        int16_t delta = static_cast<int16_t>(encoded_delta) - 7;
                        frame[s * m_channels + c] = m_prev_values[c] + delta;
                    } else {
                        // absolute
                        frame[s * m_channels + c] = get_bits(10);
                    }
                }
                m_prev_values[c] = frame[s * m_channels + c];
            }
        }

        return frame;
    }
};

} // namespace myelin

using namespace myelin;

int main() {
    int channels = 1024;
    int samples = 20000 * 5 / 1000; // 5ms @ 20kHz

    std::vector<uint16_t> frame(channels * samples);
    std::mt19937 gen(42);
    std::uniform_int_distribution<uint16_t> dist(0, 1023);
    for (auto& val : frame) {
        val = dist(gen);
    }

    BSDE encoder(channels);
    std::vector<uint8_t> encoded = encoder.encode(frame.data(), samples);

    std::cout << "Encoded size: " << encoded.size() << std::endl;

    // Benchmark decode
    int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        BSDE_New dec(channels); // Need fresh state
        auto decoded = dec.decode(encoded.data(), encoded.size(), samples);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Decode time for " << iterations << " iterations: " << duration << " ms" << std::endl;
    std::cout << "Avg Decode time per iteration: " << (double)duration / iterations << " ms" << std::endl;

    return 0;
}
