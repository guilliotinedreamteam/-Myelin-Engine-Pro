#ifndef MYELIN_ASS_HPP
#define MYELIN_ASS_HPP

#include <vector>
#include <cstdint>

namespace myelin {

/**
 * Adaptive Spatiotemporal Squelch (ASS)
 * Subtracts local average noise from clusters of channels to enhance sparsity.
 */
class ASS {
public:
    ASS(int total_channels, int cluster_size);
    
    // Applies squelch to a frame in-place.
    // Frame is row-major: samples * total_channels + channel
    void apply(uint16_t* frame, int samples);
    
    // Reconstructs original signal from squelched frame in-place
    void reconstruct(uint16_t* frame, int samples);

private:
    int m_total_channels;
    int m_cluster_size;
    std::vector<uint16_t> m_cluster_averages; // Store averages for reconstruction (normally sent separately in telemetry, but inline here for simplicity)
};

} // namespace myelin

#endif // MYELIN_ASS_HPP
