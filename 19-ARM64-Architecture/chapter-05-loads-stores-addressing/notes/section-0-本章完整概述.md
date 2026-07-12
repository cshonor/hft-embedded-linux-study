## Ch5 完整概述 · 加载、存储与寻址

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Loads, Stores, and Addressing · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **RISC 核心** | **Load → 算 → Store** — 约占动态指令一半 |
| **寻址** | Pre/Post 变址 · 寄存器移位偏移 — 数组/MMIO 基础 |
| **系统观** | Memory Map · Endian · 链接脚本放置 |

**前置：** [Ch4 伪指令](../chapter-04-assembler-rules-directives/notes/section-0-本章完整概述.md) · [Ch2 对齐](../chapter-02-programmers-model/notes/section-2-2-data-types.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **内存映射** | §5.2 | [section-5-2-memory.md](./section-5-2-memory.md) |
| **LDR/STR 族** | §5.3 | [section-5-3-load-store.md](./section-5-3-load-store.md) |
| **前/后变址** | §5.4 | [section-5-4-addressing.md](./section-5-4-addressing.md) |
| **字节序** | §5.5 | [section-5-5-endianness.md](./section-5-5-endianness.md) |
| **位带** | §5.6 | [section-5-6-bit-banded.md](./section-5-6-bit-banded.md) |
| **链接与段** | §5.7 | [section-5-7-memory-notes.md](./section-5-7-memory-notes.md) |

---

### 三、知识流（口述版）

```
Memory Map：Flash / SRAM / MMIO 各占一段地址
        ↓
LDRB/H/无符号 · LDRSB/SH 有符号 → 寄存器
        ↓
[基址 ± 偏移 !] Pre 或 [基址], #off Post — 数组/栈
        ↓
Little-endian 默认 · REV 跨序
        ↓
(M3/M4) 位带单 bit STR · 链接脚本定 RO/RW 物理地址
        ↓
Ch7 ALU · Ch16 MMIO 实战
```

---

### 四、与 HFT / 嵌入式对照

| 已学/将学 | Ch5 |
|-----------|-----|
| CSAPP 内存/指针 | 同一 Load/Store，不同助记符 |
| MikanOS MMIO | `LDR`/`STR` 写帧缓冲/端口 |
| [21 驱动](../../../21-Linux-Device-Driver/) | `readl`/`writel` = 宽度正确的 MMIO |
| [22 DT](../../../22-Device-Tree-Study/) | 基址不再硬编码 `EQU` |

---

### 五、下一章

→ **[Ch6 常量与文字池](../../chapter-06-constants-literal-pools/)**（选读）或直进 **[Ch7 整数运算](../../chapter-07-integer-logic-arithmetic/)**（精读）
