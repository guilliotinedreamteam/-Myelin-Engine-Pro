import re

with open('bsde_impl.cpp', 'r') as f:
    content = f.read()

old_get_bits = """    auto get_bits = [&](int count) -> uint32_t {
        uint32_t val = 0;
        for (int i = 0; i < count; ++i) {
            val = (val << 1) | (get_bit() ? 1 : 0);
        }
        return val;
    };"""

new_get_bits = """    auto get_bits = [&](int count) -> uint32_t {
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
    };"""

new_content = content.replace(old_get_bits, new_get_bits)

with open('bsde_impl.cpp', 'w') as f:
    f.write(new_content)
