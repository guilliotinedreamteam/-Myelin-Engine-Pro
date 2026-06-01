#include "bsde.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstring>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

namespace myelin {

struct alignas(64) BSDE_LUT {
    uint32_t len[2048];
    uint32_t bits[2048];
    uint32_t mask[2048];

    BSDE_LUT() {
        for (int d = -1024; d <= 1023; ++d) {
            if (d == 0) {
                len[d + 1024] = 1;
                bits[d + 1024] = 0;
                mask[d + 1024] = 0;
            } else if (d >= -7 && d <= 7) {
                len[d + 1024] = 6;
                bits[d + 1024] = 0x20 | static_cast<uint8_t>(d + 7);
                mask[d + 1024] = 0;
            } else {
                len[d + 1024] = 12;
                bits[d + 1024] = 0xC00;
                mask[d + 1024] = 0x3FF;
            }
        }
    }
};
static const BSDE_LUT g_lut;

BSDE::BSDE(int channels) : m_channels(channels) {
    m_prev_values.resize(channels, 0);
}

std::vector<uint8_t> BSDE::encode(const uint16_t* __restrict frame, int samples) {
    int max_bytes = (samples * m_channels * 12 + 7) / 8 + 8;
    std::vector<uint8_t> bitstream(max_bytes, 0);
    uint8_t* __restrict out_ptr = bitstream.data();
    int out_idx = 0;

    uint64_t bit_buffer = 0;
    int bits_in_buffer = 0;

    const uint16_t* __restrict in_ptr = frame;
    uint16_t* __restrict prev_vals = m_prev_values.data();
    int channels = m_channels;

    int total_samples = samples * channels;

    thread_local std::vector<int16_t> deltas;
    if (deltas.size() < static_cast<size_t>(total_samples)) {
        deltas.resize(total_samples);
    }
    int16_t* __restrict delta_ptr = deltas.data();

    // Phase A: Compute deltas across channels (s is time, c is channel)
    for (int s = 0; s < samples; ++s) {
        int offset = s * channels;
        int c = 0;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
        for (; c <= channels - 8; c += 8) {
            uint16x8_t curr = vld1q_u16(in_ptr + offset + c);
            uint16x8_t prev = vld1q_u16(prev_vals + c);

            // signed delta = current - prev
            int16x8_t delta = vsubq_s16(vreinterpretq_s16_u16(curr), vreinterpretq_s16_u16(prev));
            vst1q_s16(delta_ptr + offset + c, delta);

            vst1q_u16(prev_vals + c, curr);
        }
#endif
        for (; c < channels; ++c) {
            uint16_t curr = in_ptr[offset + c];
            delta_ptr[offset + c] = static_cast<int16_t>(curr) - static_cast<int16_t>(prev_vals[c]);
            prev_vals[c] = curr;
        }
    }

    const uint32_t* __restrict lut_len = g_lut.len;
    const uint32_t* __restrict lut_bits = g_lut.bits;
    const uint32_t* __restrict lut_mask = g_lut.mask;

    // Phase B: Branchless Bit-Packing
    for (int i = 0; i < total_samples; ++i) {
        int16_t d = delta_ptr[i];
        uint16_t current_val = in_ptr[i];

        int lut_idx = d + 1024;

        // Ensure bounds safely to avoid branches if we know limits
        lut_idx = std::max(0, std::min(2047, lut_idx));

        uint32_t len = lut_len[lut_idx];
        uint32_t bits = lut_bits[lut_idx] | (current_val & lut_mask[lut_idx]);

        bit_buffer = (bit_buffer << len) | bits;
        bits_in_buffer += len;

        // Using 64 bit writes is faster
        if (bits_in_buffer >= 32) {
            bits_in_buffer -= 32;
            uint32_t to_write = static_cast<uint32_t>(bit_buffer >> bits_in_buffer);
            to_write = __builtin_bswap32(to_write);
            std::memcpy(out_ptr + out_idx, &to_write, 4);
            out_idx += 4;
        }
    }

    int rem = bits_in_buffer;
    while (rem >= 8) {
        rem -= 8;
        out_ptr[out_idx++] = static_cast<uint8_t>(bit_buffer >> rem);
    }
    if (rem > 0) {
        out_ptr[out_idx++] = static_cast<uint8_t>(bit_buffer << (8 - rem));
    }

    bitstream.resize(out_idx);
    return bitstream;
}

std::vector<uint16_t> BSDE::decode(const uint8_t* __restrict bits, int bits_len, int samples) {
    std::vector<uint16_t> frame(m_channels * samples);
    uint16_t* __restrict out_ptr = frame.data();

    int byte_idx = 0;
    uint64_t bit_buffer = 0;
    int bits_in_buffer = 0;

    uint16_t* __restrict prev_vals = m_prev_values.data();
    int channels = m_channels;

    for (int s = 0; s < samples; ++s) {
        for (int c = 0; c < channels; ++c) {
            if (bits_in_buffer < 12) {
                while (bits_in_buffer <= 32 && byte_idx + 4 <= bits_len) {
                    uint32_t val;
                    std::memcpy(&val, bits + byte_idx, 4);
                    val = __builtin_bswap32(val);
                    bit_buffer = (bit_buffer << 32) | val;
                    bits_in_buffer += 32;
                    byte_idx += 4;
                }
                while (bits_in_buffer <= 56 && byte_idx < bits_len) {
                    bit_buffer = (bit_buffer << 8) | bits[byte_idx++];
                    bits_in_buffer += 8;
                }
            }

            bits_in_buffer--;
            bool bit1 = (bit_buffer >> bits_in_buffer) & 1;

            if (!bit1) {
                *out_ptr = prev_vals[c];
            } else {
                bits_in_buffer--;
                bool bit2 = (bit_buffer >> bits_in_buffer) & 1;

                if (!bit2) {
                    bits_in_buffer -= 4;
                    uint8_t encoded_delta = (bit_buffer >> bits_in_buffer) & 0xF;
                    int16_t delta = static_cast<int16_t>(encoded_delta) - 7;
                    *out_ptr = prev_vals[c] + delta;
                } else {
                    bits_in_buffer -= 10;
                    *out_ptr = (bit_buffer >> bits_in_buffer) & 0x3FF;
                }
            }
            prev_vals[c] = *out_ptr++;
        }
    }

    return frame;
}

} // namespace myelin
