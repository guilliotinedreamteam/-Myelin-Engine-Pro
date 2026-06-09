import re

with open('bsde_impl.cpp', 'r') as f:
    content = f.read()

old_push_bits = """    auto push_bits = [&](uint32_t value, int count) {
        for (int i = count - 1; i >= 0; --i) {
            push_bit((value >> i) & 1);
        }
    };"""

new_push_bits = """    auto push_bits = [&](uint32_t value, int count) {
        while (count > 0) {
            int bits_available = bit_pos + 1;
            int bits_to_write = std::min(count, bits_available);

            uint32_t extracted_bits = (value >> (count - bits_to_write)) & ((1 << bits_to_write) - 1);
            current_byte |= (extracted_bits << (bits_available - bits_to_write));

            bit_pos -= bits_to_write;
            if (bit_pos < 0) {
                bitstream.push_back(current_byte);
                current_byte = 0;
                bit_pos = 7;
            }
            count -= bits_to_write;
        }
    };"""

new_content = content.replace(old_push_bits, new_push_bits)

with open('bsde_impl.cpp', 'w') as f:
    f.write(new_content)
