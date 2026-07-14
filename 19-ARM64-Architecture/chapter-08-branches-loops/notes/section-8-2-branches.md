## §8.2 分支机制 — B · BX · BL · CBZ/CBNZ

> **Ch 8 · 分支与循环** · [章导读](../README.md)

---

### 流水线与分支代价

```
取指 → 译码 → 执行 …
         ↑
    分支改 PC → 前面已预取的指令作废（pipeline flush / bubble）
```

**软件** 必须用分支实现 if/loop；**优化** = 少分支、可预测分支、用条件执行代替短分支。

---

### ARM7TDMI（v4T · ARM/Thumb 状态）

| 指令 | 作用 |
|------|------|
| **`B label`** | 直接跳转；范围约 **±32MB**（ARM 模式） |
| **`B{cond} label`** | **条件分支** — 读 CPSR N,Z,C,V（→ [Ch7 §7.2](../../chapter-07-integer-logic-arithmetic/notes/section-7-2-flags.md)） |
| **`BX Rm`** | **间接跳转**；Rm 的 **LSB** 决定 ARM/Thumb 状态（→ **Ch17**） |
| **`BL label`** | **Branch with Link** — **LR ← 返回地址**，跳子程序（→ **Ch13**） |
| **`BL{cond}`** | 条件调用 |

**返回：** `BX lr` 或 `MOV pc, lr`（模式相关）。

---

### Cortex-M4（v7-M · Thumb-2 only）

| 指令 | 作用 |
|------|------|
| **`B` / `B{cond}`** | 32 bit 分支约 **±16MB** |
| **`BX` / `BLX Rm`** | 间接跳；**BLX** = 带链接的间接分支 |
| **`BL`** | 调用；返回地址在 **LR**，异常时 LR 为 **EXC_RETURN**（Ch2） |

**无 ARM 32 bit 状态** — BX 的 LSB 仍标记 Thumb，但始终 Thumb-2。

---

### CBZ / CBNZ — M 系循环优化

```asm
        CBZ     r0, done          ; if r0==0 → done（不改 APSR）
        CBNZ    r1, loop_top      ; if r1!=0 → loop_top
```

| 属性 | 说明 |
|------|------|
| **范围** | 短跳 **约 4–130 字节**（向前/向后） |
| **寄存器** | 仅 **r0–r7** |
| **标志** | **不更新** N,Z,C,V — 与后续 `CMP` 独立 |
| **限制** | **不能在 IT 块内** 使用 |

**用途：** 紧凑 inner loop 测试 — 省一条 `CMP`+`BEQ`。

---

### 常见条件后缀（与 CMP 配对）

| 组 | 后缀 | 何时用 |
|----|------|--------|
| 相等 | **EQ / NE** | Z=1 / Z=0 |
| 有符号 | **GT / GE / LT / LE** | 可负整数比较 |
| 无符号 | **HI / HS / LO / LS** | 地址、长度、size_t |

**日常背三个：** **EQ / GT / LT**（有符号）。完整口述表 → [Ch7 §7.2](../../chapter-07-integer-logic-arithmetic/notes/section-7-2-flags.md)。

**Ch3 阶乘** 已用 **GT** / **IT**。

---

### 可复述要点

1. **`BL` 调子程序写 LR**；**`BX`/`BLX` 间接跳**。  
2. **条件分支** = `B` + 条件码，消费 **Ch7 标志**。  
3. **`CBZ/CBNZ`** = 短跳、不改标志、仅 r0–r7、禁 IT 块内。
