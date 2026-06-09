#include "telemetry_impl.cpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

using namespace myelin;

void run_test() {
    uint64_t seed = 0x1337BEEF;
    int port = 55555;

    try {
        QER_Receiver receiver(port, seed);
        QER_Transmitter transmitter("127.0.0.1", port, seed);

        std::vector<uint8_t> pure_entropy = {0xDE, 0xAD, 0xBE, 0xEF};

        int bytes_sent = transmitter.transmit_entropy(pure_entropy);
        assert(bytes_sent == 4);

        std::vector<uint8_t> entropy_out;
        bool decoherence_detected = false;

        bool received = receiver.receive_entropy(entropy_out, decoherence_detected, 1000);
        assert(received == true);
        assert(decoherence_detected == false);
        assert(entropy_out == pure_entropy);

        std::cout << "[Test] Transmit and receive successful." << std::endl;

        // Test timeout (decoherence)
        entropy_out.clear();
        received = receiver.receive_entropy(entropy_out, decoherence_detected, 10);
        assert(received == false);
        assert(decoherence_detected == true);

        std::cout << "[Test] Timeout (decoherence) test successful." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Test Error] " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "--- QER Telemetry Protocol Test ---" << std::endl;
    run_test();
    std::cout << "QER Core Test Passed!" << std::endl;
    return 0;
}
