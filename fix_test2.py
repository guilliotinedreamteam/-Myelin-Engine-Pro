with open('test_bsde_core.cpp', 'r') as f:
    content = f.read()
content = content.replace('#include "bsde.cpp"', '#include "bsde_impl.cpp"\n#include "bsde.hpp"')
with open('test_bsde_core.cpp', 'w') as f:
    f.write(content)
