## §17.3 32 位 Thumb 指令 (Thumb-2)

> **Ch 17 · ARM、Thumb 和 Thumb-2 指令** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 为何需要 Thumb-2（2003）

| 16-bit Thumb 痛点 | Thumb-2 解决 |
|-------------------|--------------|
| 寄存器/立即数 **太窄** | **32-bit Thumb** 指令恢复能力 |
| ARM↔Thumb **频繁 BX** | M 系 **不再切 ARM** — 单 ISA 内混合宽度 |
| 中断路径 **状态切换开销** | Cortex-M **纯 Thumb-2** — [Ch15](../chapter-15-exception-handling-v7m/) |

**Thumb-2 = 超集：** **保留 16-bit Thumb** + **新增 32-bit Thumb** — 同一代码流 **可变长**。

---

### Cortex-M3/M4：只执行 Thumb-2

```
无 ARM 32-bit 状态
无 CPSR.T 切换
取指永远是 Thumb 长度检测 → 16 或 32 bit
```

本书 **Ch15–16 M4 例程** 均属 Thumb-2（含 **CBZ、IT、32-bit BL** 等）。

---

### 如何区分 16 vs 32 bit 指令

取指后看 **半字高 5 bit**：

| 高 5 bit | 含义 |
|----------|------|
| **≠ 11101, 11110, 11111** | **16-bit Thumb**（一条半字） |
| **`11101` / `11110` / `11111`** | **32-bit Thumb** 的第一或第二半字 |

CPU **连续取两个半字** 拼成 32-bit 指令 — 对程序员 **汇编器自动选长度**。

---

### UAL — Unified Assembly Language

Thumb-2 推出 **统一汇编语法** — ARM 与 Thumb **同一套助记符规则**：

| 规则 | 说明 |
|------|------|
| **更新标志** | 必须 **显式 `S` 后缀** — **`ADDS`** |
| **不更新标志** | **无 S** — 汇编器可能选 **较长 32-bit** 编码实现 |
| **条件** | 16-bit 无 `{cond}` → 用 **IT 块**（[Ch8 §8.4](../chapter-08-branches-loops/notes/section-8-4-conditional.md)） |

**口述：** UAL 下 **Thumb-2 标志语义对齐 ARM** — **要改标志才加 S**；与 **老 16-bit Thumb「默认改标志」** 不同，写 M4 以 **UAL 为准**。

---

### Thumb-2 能力举例

| 能力 | 说明 |
|------|------|
| **全寄存器** | r0–r12 等 — 不再限 r0–r7 |
| **大立即数 / 宽分支** | 32-bit 编码 |
| **IT + 条件** | 替代 ARM `{cond}` 数据指令 |
| **硬件除法** | M3/M4 **SDIV/UDIV** |
| **DSP 类** | 部分 SIMD 指令 |

---

### 可复述要点

1. **Thumb-2 = 16 + 32 混合**，M4 **唯一 ISA**。  
2. **高 5 bit 1110x/1111x** → 32-bit 指令对。  
3. **UAL：`S` 后缀管标志** — 与 legacy 16-bit Thumb 习惯不同。
