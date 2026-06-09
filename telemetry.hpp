#ifndef MYELIN_QER_TELEMETRY_HPP
#define MYELIN_QER_TELEMETRY_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <arpa/inet.h>
#include <random>

namespace myelin {



struct ChaCha20StateGenerator {
    uint32_t state[16];
    uint64_t buffer[8];
    int buffer_idx;

    explicit ChaCha20StateGenerator(uint64_t seed) {
        state[0] = 0x61707865;
        state[1] = 0x3320646e;
        state[2] = 0x79622d32;
        state[3] = 0x6b206574;

        state[4] = static_cast<uint32_t>(seed & 0xFFFFFFFF);
        state[5] = static_cast<uint32_t>(seed >> 32);
        for (int i = 6; i < 16; ++i) {
            state[i] = 0;
        }
        buffer_idx = 8; // Force generation on first call
    }

    typedef uint64_t result_type;
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT64_MAX; }

    uint64_t operator()() {
        if (buffer_idx >= 8) {
            generate_block();
            buffer_idx = 0;
        }
        return buffer[buffer_idx++];
    }

private:
    void generate_block() {
        state[12]++;
        if (state[12] == 0) {
            state[13]++;
        }

        uint32_t x[16];
        for (int i = 0; i < 16; ++i) x[i] = state[i];

        #define ROTL(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
        #define QR(a, b, c, d) \
            x[a] += x[b]; x[d] ^= x[a]; x[d] = ROTL(x[d], 16); \
            x[c] += x[d]; x[b] ^= x[c]; x[b] = ROTL(x[b], 12); \
            x[a] += x[b]; x[d] ^= x[a]; x[d] = ROTL(x[d], 8); \
            x[c] += x[d]; x[b] ^= x[c]; x[b] = ROTL(x[b], 7);

        for (int i = 0; i < 10; ++i) {
            QR(0, 4, 8, 12); QR(1, 5, 9, 13); QR(2, 6, 10, 14); QR(3, 7, 11, 15);
            QR(0, 5, 10, 15); QR(1, 6, 11, 12); QR(2, 7, 8, 13); QR(3, 4, 9, 14);
        }
        #undef QR
        #undef ROTL

        for (int i = 0; i < 16; ++i) {
            uint32_t out = state[i] + x[i];
            if (i % 2 == 0) {
                buffer[i/2] = static_cast<uint64_t>(out);
            } else {
                buffer[i/2] |= (static_cast<uint64_t>(out) << 32);
            }
        }
    }
};

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

protected:
    int m_socket;
    struct sockaddr_in m_addr;
    
    // The "Entangled" State Machine
    uint64_t m_shared_seed;
    uint32_t m_expected_frame_seq;
    ChaCha20StateGenerator m_state_generator; // Simulating the quantum state evolution
    
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