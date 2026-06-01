## 2024-06-01 - [Sub-1ms Telemetry Encoding]
**Learning:** For extremely tight inner loops (1024 channels at 20kHz), `if/else` branching for bit manipulation causes severe CPU pipeline flushes.
**Action:** Replace sequential if/else delta checks with a 4KB branchless static L1-cached Look-Up Table (LUT) mapping deltas to lengths and bits. Combine this with SIMD (`arm_neon.h`) to compute the deltas initially.
