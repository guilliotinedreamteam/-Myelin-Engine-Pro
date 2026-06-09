#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>

int main() {
    std::vector<uint8_t> bits = {0b11010110, 0b10110101, 0b00111100};
    int bits_len = 3;
    int byte_idx = 0;
    int bit_idx = 0;

    auto get_bit = [&]() -> bool {
        if (byte_idx >= bits_len) return false;
        bool bit = (bits[byte_idx] >> (7 - bit_idx)) & 1;
        bit_idx++;
        if (bit_idx > 7) {
            bit_idx = 0;
            byte_idx++;
        }
        return bit;
    };

    auto get_bits_old = [&](int count) -> uint32_t {
        uint32_t val = 0;
        for (int i = 0; i < count; ++i) {
            val = (val << 1) | (get_bit() ? 1 : 0);
        }
        return val;
    };

    byte_idx = 0;
    bit_idx = 0;

    auto get_bits_new = [&](int count) -> uint32_t {
        uint32_t val = 0;
        while (count > 0) {
            if (byte_idx >= bits_len) return val << count;

            int bits_available = 8 - bit_idx;
            int bits_to_read = std::min(count, bits_available);

            uint32_t extracted_bits = (bits[byte_idx] >> (bits_available - bits_to_read)) & ((1 << bits_to_read) - 1);

            val = (val << bits_to_read) | extracted_bits;

            bit_idx += bits_to_read;
            if (bit_idx >= 8) {
                bit_idx = 0;
                byte_idx++;
            }

            count -= bits_to_read;
        }
        return val;
    };

    int b_idx_old = 0, bi_idx_old = 0;
    int b_idx_new = 0, bi_idx_new = 0;

    auto test = [&](int count) {
        byte_idx = b_idx_old; bit_idx = bi_idx_old;
        uint32_t old_val = get_bits_old(count);
        b_idx_old = byte_idx; bi_idx_old = bit_idx;

        byte_idx = b_idx_new; bit_idx = bi_idx_new;
        uint32_t new_val = get_bits_new(count);
        b_idx_new = byte_idx; bi_idx_new = bit_idx;

        std::cout << "Read " << count << " bits: Old=" << old_val << " New=" << new_val
                  << " Match=" << (old_val == new_val) << std::endl;
    };

    test(3); // 110 (6)
    test(5); // 10110 (22)
    test(4); // 1011 (11)
    test(10); // 0101001111 (335)
    test(10); // goes out of bounds

    return 0;
}
