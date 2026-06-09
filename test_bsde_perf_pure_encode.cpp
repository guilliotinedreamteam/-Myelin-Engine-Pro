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

    // Benchmark encode
    int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        BSDE enc(channels); // Need fresh state
        auto encoded = enc.encode(frame.data(), samples);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Encode time for " << iterations << " iterations: " << duration << " ms" << std::endl;
    std::cout << "Avg Encode time per iteration: " << (double)duration / iterations << " ms" << std::endl;

    return 0;
}
