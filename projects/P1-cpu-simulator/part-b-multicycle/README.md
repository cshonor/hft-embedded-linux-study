# P1 part-b — 多周期 8-bit CPU

> 卡住翻：[3.4 FSM](../../../00-digital-logic-cpu/ch03_sequential/3.4_有限状态机.md) · [7.3 单周期对照](../../../00-digital-logic-cpu/ch07_microarchitecture/7.3_单周期处理器.md) · [6.4 机器语言](../../../00-digital-logic-cpu/ch06_architecture/6.4_机器语言.md)

ALU 来自 [part-a](../part-a-alu-host/)。这里把零件收成 **取指 → 译码 → 执行 → 写回** 的 Moore 风格控制器。不是单周期一拍走完，故意拆拍，才能按周期讲信号。

源码里有给新手的中文注释：先看 `cpu.h` 里 `Cpu` 每个字段，再看 `cpu.c` 的 `cpu_clock`（一拍只做一件事），指令怎么编码看 `isa.h` 顶部那张位图。

```bash
make test
./cpu_sim --trace sum
```

Windows：

```bash
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/12392/Desktop/hft/projects/P1-cpu-simulator/part-b-multicycle && make test'
```

## 数据通路

```
PC → IMem(16-bit 指令) → IR → 译码
                              ↓
                         RegFile R0–R3
                              ↓
                            ALU ──► 写回
                              ↓
                         DMem(256×8)
```

每条指令 4 拍（HALT 在译码拍停机）。

## 三个程序（交付物）

| 程序 | 做什么 | 断言 |
|------|--------|------|
| `sum` | R0 = 5+4+3+2+1 | R0==15 |
| `fib` | 迭代到 F7 | R1==13 |
| `memcpy` | 拷 4 字节 `0x10 → 0x20` | 目的地 AA BB CC DD |

## 指令一览

16-bit：`op[15:12] rd[11:10] rs[9:8] rt[7:6] / imm[7:0]`

MOVI / ADDI / SUBI / J* 走 imm；ADD / LOAD / STORE / MOV 走寄存器。
