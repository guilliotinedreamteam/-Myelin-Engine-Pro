#include "bsde.hpp"
#include <iostream>
#include <algorithm>

namespace myelin {

BSDE::BSDE(int channels) : m_channels(channels) {
    m_prev_values.resize(channels, 0);
}

std::vector<uint8_t> BSDE::encode(const uint16_t* frame, int samples) {
    // Variable-length bitstream implementation
    // Format: 0 = no change, 1 + [4-bit signed delta] = small change, 1 + 1111 + [10-bit absolute] = large change
    
    std::vector<uint8_t> bitstream;
    bitstream.reserve(samples * m_channels); // pre-allocate approximate space
    uint8_t current_byte = 0;
    int bit_pos = 7;

    auto push_bit = [&](bool bit) {
        if (bit) current_byte |= (1 << bit_pos);
        bit_pos--;
        if (bit_pos < 0) {
            bitstream.push_back(current_byte);
            current_byte = 0;
            bit_pos = 7;
        }
    };

    auto push_bits = [&](uint32_t value, int count) {
        while (count > 0) {
            int bits_available = bit_pos + 1;
            int bits_to_write = std::min(count, bits_available);

            uint32_t extracted_bits = (value >> (count - bits_to_write)) & ((1 << bits_to_write) - 1);
            current_byte |= (extracted_bits << (bits_available - bits_to_write));

            bit_pos -= bits_to_write;
            if (bit_pos < 0) {
                bitstream.push_back(current_byte);
                current_byte = 0;
                bit_pos = 7;
            }
            count -= bits_to_write;
        }
    };

    for (int s = 0; samples > s; ++s) {
        for (int c = 0; c < m_channels; ++c) {
            uint16_t current_val = frame[s * m_channels + c];
            int16_t delta = static_cast<int16_t>(current_val) - static_cast<int16_t>(m_prev_values[c]);
            
            if (delta == 0) {
                push_bit(0); // No change
            } else if (delta >= -7 && delta <= 7) {
                push_bit(1);
                push_bit(0); // 0 prefix for small delta
                uint8_t encoded_delta = static_cast<uint8_t>(delta + 7); // Offset to 0-14
                push_bits(encoded_delta, 4);
            } else {
                push_bit(1);
                push_bit(1); // 1 prefix for absolute
                push_bits(current_val, 10);
            }
            m_prev_values[c] = current_val;
        }
    }

    if (bit_pos < 7) {
        bitstream.push_back(current_byte);
    }

    return bitstream;
}

std::vector<uint16_t> BSDE::decode(const uint8_t* bits, int bits_len, int samples) {
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

} // namespace myelin
