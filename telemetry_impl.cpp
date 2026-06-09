#include "telemetry.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <stdexcept>
#include <poll.h>
#include <chrono>

namespace myelin {

static uint64_t compute_mac(const uint8_t* data, size_t len, uint64_t seed, uint32_t seq) {
    uint64_t hash = 0xcbf29ce484222325ULL ^ seed ^ seq;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// --- QER_Node ---


QER_Node::QER_Node(int port, uint64_t entanglement_seed) 
    : m_shared_seed(entanglement_seed), m_expected_frame_seq(0), m_state_generator(entanglement_seed) {
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        throw std::runtime_error("Failed to create UDP socket for QER");
    }
}

QER_Node::~QER_Node() {
    if (m_socket >= 0) {
        close(m_socket);
    }
}

void QER_Node::evolve_state() {
    // In a true QER system, this state evolution calculates the expected
    // sparsity and noise baselines for the specific upcoming microsecond.
    // We simulate this by advancing the PRNG state.
    m_expected_frame_seq++;
    m_state_generator(); 
}

// --- QER_Transmitter ---

QER_Transmitter::QER_Transmitter(const std::string& target_ip, int target_port, uint64_t entanglement_seed) 
    : QER_Node(0, entanglement_seed) {
    
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(target_port);
    if (inet_pton(AF_INET, target_ip.c_str(), &m_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid QER target IP address");
    }
}

int QER_Transmitter::transmit_entropy(const std::vector<uint8_t>& pure_entropy) {
    // ZERO HEADER TRANSMISSION. We send ONLY the raw, compressed bits.
    // The receiver already knows what frame this is via state entanglement.
    std::vector<uint8_t> payload = pure_entropy;
    uint64_t mac = compute_mac(pure_entropy.data(), pure_entropy.size(), m_shared_seed, m_expected_frame_seq);
    uint8_t* mac_ptr = reinterpret_cast<uint8_t*>(&mac);
    payload.insert(payload.end(), mac_ptr, mac_ptr + sizeof(mac));

    int bytes_sent = sendto(m_socket, payload.data(), payload.size(), 0,
                            (struct sockaddr*)&m_addr, sizeof(m_addr));
    evolve_state();
    return bytes_sent > 0 ? (bytes_sent > 8 ? bytes_sent - 8 : 0) : bytes_sent;
}

// --- QER_Receiver ---

QER_Receiver::QER_Receiver(int listen_port, uint64_t entanglement_seed) 
    : QER_Node(listen_port, entanglement_seed) {
    
    int opt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_addr.sin_port = htons(listen_port);
    
    if (bind(m_socket, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
        throw std::runtime_error("Failed to bind QER socket to port");
    }
}

bool QER_Receiver::receive_entropy(std::vector<uint8_t>& entropy_out, bool& decoherence_detected, int timeout_ms) {
    struct pollfd fd;
    fd.fd = m_socket;
    fd.events = POLLIN;
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        int remaining_timeout = timeout_ms - elapsed_ms;
        if (remaining_timeout < 0) remaining_timeout = 0;

        // In BMI telemetry, late data is dead data. The timeout IS the clock.
        int ret = poll(&fd, 1, remaining_timeout);
        if (ret <= 0) {
            // Timeout means the frame was lost. Decoherence!
            // We advance the state machine blindly to attempt re-sync on the next tick.
            decoherence_detected = true;
            evolve_state();
            return false;
        }

        uint8_t buffer[65535];
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);

        ssize_t bytes_read = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                      (struct sockaddr*)&sender_addr, &sender_len);

        if (bytes_read >= 8) {
            uint64_t received_mac;
            std::memcpy(&received_mac, buffer + bytes_read - 8, 8);
            uint64_t expected_mac = compute_mac(buffer, bytes_read - 8, m_shared_seed, m_expected_frame_seq);
            if (received_mac == expected_mac) {
                decoherence_detected = false;
                entropy_out.assign(buffer, buffer + bytes_read - 8);
                evolve_state();
                return true;
            }
        }
    }
}

} // namespace myelin
