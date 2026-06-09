#include "ass_impl.cpp"
#include <cassert>
#include <iostream>
#include <vector>

int main() {
    int total_channels = 8;
    int cluster_size = 4;
    int samples = 2;
    myelin::ASS squelch(total_channels, cluster_size);

    std::vector<uint16_t> original_frame = {
        // Sample 0: Cluster 0 has avg 500, Cluster 1 has avg 600
        500, 502, 498, 500,  600, 602, 598, 600,
        // Sample 1: Cluster 0 has avg 510, Cluster 1 has avg 610
        510, 512, 508, 510,  610, 612, 608, 610
    };

    std::vector<uint16_t> frame = original_frame;

    squelch.apply(frame.data(), samples);

    // Verify centering around 512
    // Sample 0, Cluster 0: orig 500, avg 500 -> 500 - 500 + 512 = 512
    if (frame[0] != 512) {
        std::cerr << "ASS Test Failed: expected 512, got " << frame[0] << std::endl;
        return 1;
    }
    // Sample 0, Cluster 0, item 1: orig 502, avg 500 -> 502 - 500 + 512 = 514
    if (frame[1] != 514) {
        std::cerr << "ASS Test Failed: expected 514, got " << frame[1] << std::endl;
        return 1;
    }

    squelch.reconstruct(frame.data(), samples);

    // Verify reconstruction
    for (size_t i = 0; i < frame.size(); ++i) {
        if (frame[i] != original_frame[i]) {
            std::cerr << "Reconstruction Failed at index " << i << ": expected " << original_frame[i] << ", got " << frame[i] << std::endl;
            return 1;
        }
    }

    std::cout << "ASS Core Test Passed!" << std::endl;
    return 0;
}
