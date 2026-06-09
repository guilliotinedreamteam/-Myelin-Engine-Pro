with open('test_bsde_core.cpp', 'r') as f:
    content = f.read()
content = content.replace('encoder.encode(frame, samples)', 'encoder.encode(frame.data(), samples)')
content = content.replace('decoder.decode(bits, samples)', 'decoder.decode(bits.data(), bits.size(), samples)')
with open('test_bsde_core.cpp', 'w') as f:
    f.write(content)
