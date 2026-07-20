# Harris ARM 第2版 — one-page summary

## Eight chapters (your edition)

```
1 Binary → 2 Combo(MUX/ALU) → 3 Seq(FF)
                ↓ skip 4 HDL
5 ARM ISA → 6 Mem+I/O(Cache) → 7 Microarch(pipe) → 8 RPi
                ↕ embedded              ↕ CSAPP SEQ/PIPE
```

## Must vs skip

- **精读：** 2, 3, 5, 6(cache), **7**
- **浅读：** 1, 6(peripherals), 8
- **跳过：** 4 + 所有 Verilog 大段
