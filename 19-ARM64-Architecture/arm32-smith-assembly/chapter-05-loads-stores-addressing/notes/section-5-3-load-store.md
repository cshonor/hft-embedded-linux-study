## §5.3 加载与存储指令 — LDR / STR 族

> **Ch 5 · 加载、存储与寻址** · [章导读](../README.md)

---

### RISC 铁律

```
内存 ──LDR──→ 寄存器 ──ALU──→ 寄存器 ──STR──→ 内存
```

**不能直接** `ADD [mem1], [mem2]`（与 x86 对比 → [Ch1 §1.2 RISC](../../chapter-01-overview-computing-systems/notes/section-1-2-risc-history.md)）。

---

### 按宽度的指令对

| 宽度 | Load | Store | 寄存器效果 |
|------|------|-------|------------|
| **字 32 bit** | **`LDR`** | **`STR`** | 整字 |
| **半字 16 bit** | **`LDRH`** | **`STRH`** | 低 16 bit（高 16 见符号扩展） |
| **字节 8 bit** | **`LDRB`** | **`STRB`** | 低 8 bit |

**语法骨架：**

```asm
        LDR     r0, [r1]          ; r0 ← Mem32[r1]
        STRB    r2, [r3, #4]      ; Mem8[r3+4] ← r2 低 8 位
```

---

### 带符号扩展 Load

读取 **有符号** 8/16 bit 到 32 bit 寄存器时，需 **符号扩展**（MSB 复制到高半部）：

| 指令 | 作用 |
|------|------|
| **`LDRSB`** | 有符号 **字节** → 32 bit |
| **`LDRSH`** | 有符号 **半字** → 32 bit |

```
Mem8 = 0xFF  (有符号 −1)
LDRB  → r0 = 0x000000FF  (255 无符号)
LDRSB → r0 = 0xFFFFFFFF  (−1 有符号) ✓
```

**C 对照：** `(int8_t)byte` vs `(uint8_t)byte` — 汇编要选对接指令。

**无符号半字/字节：** `LDRH`/`LDRB` 零扩展高位。

---

### MMIO 宽度（预告 Ch16）

外设寄存器常为 **32 bit 字** — 用 **`LDR`/`STR` 字访问**；若手册规定 **半字/字节** 必须匹配，否则 **UNPREDICTABLE** 或 bus fault。

---

### AArch64 对照（奔跑吧 Ch3）

| v4T/v7-M | AArch64 |
|----------|---------|
| `LDR r0, [r1]` | `LDR W0, [X1]` / `LDR X0, [X1]` |
| `LDRB`/`LDRH` | `LDRB`/`LDRH` + `SXTB`/`SXTH` 或专用加载 |

---

### 可复述要点

1. **`LDR*` 进寄存器，`STR*` 出寄存器** — 成对记忆。  
2. **有符号小数据 → `LDRSB`/`LDRSH`**；无符号 → `LDRB`/`LDRH`。  
3. MMIO 先查手册 **寄存器宽度**，再选对指令。
