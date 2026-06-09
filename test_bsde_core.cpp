#include "bsde_impl.cpp"
#include <cassert>
#include <iostream>
#include <vector>

int main() {
    int channels = 4;
    int samples = 10;
    myelin::BSDE encoder(channels);
    myelin::BSDE decoder(channels);

    std::vector<uint16_t> frame(channels * samples);
    for (int i = 0; i < frame.size(); ++i) {
        frame[i] = (i % 2 == 0) ? 500 : 502; // Small deltas
    }

    std::vector<uint8_t> bits = encoder.encode(frame.data(), samples);
    std::vector<uint16_t> decoded = decoder.decode(bits.data(), bits.size(), samples);

    for (int i = 0; i < frame.size(); ++i) {
        if (frame[i] != decoded[i]) {
            std::cerr << "Mismatch at index " << i << ": expected " << frame[i] << ", got " << decoded[i] << std::endl;
            return 1;
        }
    }

    std::cout << "BSDE Core Test Passed! Compressed size: " << bits.size() << " bytes (Original: " << frame.size() * 2 << " bytes)" << std::endl;
    return 0;
}
