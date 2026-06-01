#ifndef MYELIN_QER_TELEMETRY_HPP
#define MYELIN_QER_TELEMETRY_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <arpa/inet.h>
#include <random>

namespace myelin {

/**
 * Quantum Entanglement Routing (QER) Simulator
 *
 * Bypasses traditional networking overhead by maintaining tightly coupled
 * deterministic state machines (DSMs) on both transmitter and receiver.
 * Zero-header transmission. Only pure entropy is sent over the wire.
 */
class QER_Node {
public:
    QER_Node(int port, uint64_t entanglement_seed);
    virtual ~QER_Node();

    // Prevent implicit socket copying to avoid double-closes
    QER_Node(const QER_Node&) = delete;
    QER_Node& operator=(const QER_Node&) = delete;

    // Proper move semantics
    QER_Node(QER_Node&& other) noexcept;
    QER_Node& operator=(QER_Node&& other) noexcept;

protected:
    int m_socket;
    struct sockaddr_in m_addr;

    // The "Entangled" State Machine
    uint64_t m_shared_seed;
    uint32_t m_expected_frame_seq;
    std::mt19937_64 m_state_generator; // Simulating the quantum state evolution

    // Advances the state machine synchronously
    void evolve_state();
};

class QER_Transmitter : public QER_Node {
public:
    QER_Transmitter(const std::string& target_ip, int target_port, uint64_t entanglement_seed);

    // Transmits pure entropy (no headers). State evolution implicitly tracks sequence.
    int transmit_entropy(const std::vector<uint8_t>& pure_entropy);
};

class QER_Receiver : public QER_Node {
public:
    QER_Receiver(int listen_port, uint64_t entanglement_seed);

    // Receives entropy and verifies state coherence.
    // Returns true if decoherence (packet loss) is detected.
    bool receive_entropy(std::vector<uint8_t>& entropy_out, bool& decoherence_detected, int timeout_ms = 2);
};

} // namespace myelin

#endif // MYELIN_QER_TELEMETRY_HPP
