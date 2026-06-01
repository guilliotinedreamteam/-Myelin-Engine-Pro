#ifndef MYELIN_BSDE_HPP
#define MYELIN_BSDE_HPP

#include <vector>
#include <cstdint>

namespace myelin {

/**
 * Bit-Stream Delta Encoder (BSDE)
 * Optimized for sparse 10-bit neural data.
 */
class BSDE {
public:
    BSDE(int channels);

    // Encodes a frame of data (channels x samples)
    std::vector<uint8_t> encode(const uint16_t* frame, int samples);

    // Decodes a bitstream back to (channels x samples)
    std::vector<uint16_t> decode(const uint8_t* bits, int bits_len, int samples);

private:
    int m_channels;
    std::vector<uint16_t> m_prev_values;
};

} // namespace myelin

#endif // MYELIN_BSDE_HPP
