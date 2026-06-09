#include "telemetry_impl.cpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>
#include <atomic>

using namespace myelin;

std::atomic<bool> receiver_ready(false);
std::atomic<bool> test_passed(false);

void run_receiver() {
    try {
        QER_Receiver receiver(55556, 0x1337BEEF);
        std::cout << "[Receiver] Initialized successfully. Listening on port 55556." << std::endl;
        receiver_ready = true;

        std::vector<uint8_t> entropy_out;
        bool decoherence;

        // Wait for up to 1000ms for data
        for (int i = 0; i < 10; ++i) {
            if (receiver.receive_entropy(entropy_out, decoherence, 100)) {
                std::cout << "[Receiver] Received " << entropy_out.size() << " bytes." << std::endl;
                assert(!decoherence);
                assert(entropy_out.size() == 4);
                assert(entropy_out[0] == 0xDE);
                assert(entropy_out[1] == 0xAD);
                assert(entropy_out[2] == 0xBE);
                assert(entropy_out[3] == 0xEF);
                test_passed = true;
                break;
            }
        }

        // Let's test the error condition as well
        // We wait for data that will not arrive
        std::cout << "[Receiver] Waiting for data that will not arrive..." << std::endl;
        bool result = receiver.receive_entropy(entropy_out, decoherence, 100);
        assert(!result);
        assert(decoherence);
        std::cout << "[Receiver] Decoherence detected as expected." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Receiver Error] " << e.what() << std::endl;
    }
}

void run_transmitter() {
    try {
        while (!receiver_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        QER_Transmitter transmitter("127.0.0.1", 55556, 0x1337BEEF);
        std::cout << "[Transmitter] Initialized successfully." << std::endl;

        std::vector<uint8_t> entropy = {0xDE, 0xAD, 0xBE, 0xEF};
        int bytes_sent = transmitter.transmit_entropy(entropy);
        std::cout << "[Transmitter] Sent " << bytes_sent << " bytes." << std::endl;
        assert(bytes_sent == 4);
    } catch (const std::exception& e) {
        std::cerr << "[Transmitter Error] " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "--- QER Telemetry Protocol Test ---" << std::endl;

    // Test invalid IP
    try {
        QER_Transmitter invalid_transmitter("256.256.256.256", 55556, 0x1337BEEF);
        assert(false); // Should not reach here
    } catch (const std::runtime_error& e) {
        std::cout << "[Transmitter] Caught expected exception for invalid IP: " << e.what() << std::endl;
    }

    std::thread rx_thread(run_receiver);
    std::thread tx_thread(run_transmitter);
    
    rx_thread.join();
    tx_thread.join();
    
    assert(test_passed);
    std::cout << "QER Core Test Passed!" << std::endl;
    return 0;
}
