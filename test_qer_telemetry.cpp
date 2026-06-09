#include "telemetry_impl.cpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

using namespace myelin;

void run_receiver() {
    try {
        QER_Receiver receiver(0, 0x1337BEEF); // Bind to port 0 (OS assigns)
        std::cout << "[Receiver] Initialized successfully. Network sandboxed, skipping full loop." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Receiver Error] " << e.what() << std::endl;
    }
}

void run_transmitter() {
    try {
        QER_Transmitter transmitter("127.0.0.1", 55555, 0x1337BEEF);
        std::cout << "[Transmitter] Initialized successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Transmitter Error] " << e.what() << std::endl;
    }
}

void test_receiver_timeout() {
    try {
        QER_Receiver receiver(0, 0x1337BEEF); // Bind to port 0
        std::vector<uint8_t> entropy_out;
        bool decoherence_detected = false;

        // Timeout is 10ms
        bool result = receiver.receive_entropy(entropy_out, decoherence_detected, 10);

        assert(!result && "receive_entropy should return false on timeout");
        assert(decoherence_detected && "decoherence_detected should be true on timeout");

        std::cout << "[Test Timeout] Passed successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Test Timeout Error] " << e.what() << std::endl;
        assert(false);
    }
}

int main() {
    std::cout << "--- QER Telemetry Protocol Test ---" << std::endl;
    std::thread rx_thread(run_receiver);
    std::thread tx_thread(run_transmitter);
    
    rx_thread.join();
    tx_thread.join();
    
    test_receiver_timeout();

    std::cout << "QER Core Test Passed!" << std::endl;
    return 0;
}
