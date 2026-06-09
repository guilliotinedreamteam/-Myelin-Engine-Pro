#include "bsde_impl.cpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>

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

    BSDE decoder(channels);

    // Benchmark decode
    int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        BSDE dec(channels); // Need fresh state
        auto decoded = dec.decode(encoded.data(), encoded.size(), samples);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Decode time for " << iterations << " iterations: " << duration << " ms" << std::endl;
    std::cout << "Avg Decode time per iteration: " << (double)duration / iterations << " ms" << std::endl;

    return 0;
}
