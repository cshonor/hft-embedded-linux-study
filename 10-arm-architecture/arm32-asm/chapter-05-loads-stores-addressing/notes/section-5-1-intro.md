## §5.1 简介

> **Ch 5 · 加载、存储与寻址** · [章导读](../README.md)

---

### 本章在全书中的位置

| | |
|---|---|
| **角色** | **精读** — RISC/ARM 的 **核心习惯**：**Load → 算 → Store** |
| **量级** | 动态指令里约 **一半** 是 Load/Store — 性能与驱动都绕不开 |
| **前置** | [Ch2 §2.2 数据宽度与对齐](../../chapter-02-programmers-model/notes/section-2-2-data-types.md) · [Ch4 伪指令与段](../../chapter-04-assembler-rules-directives/notes/section-0-本章完整概述.md) |

---

### 六主题骨架

```
§5.2  内存 · 内存映射（Tiva 例）
§5.3  LDR/STR 族 · 符号扩展 LDRSH/LDRSB
§5.4  前变址 / 后变址 · 偏移立即数或寄存器移位
§5.5  大端 / 小端 · REV
§5.6  位带 Bit-Banding（Cortex-M 特色，Linux 路径可略）
§5.7  链接脚本 · Flash/RAM 放置
```

---

### 与 C / 驱动的关系

```c
x = arr[i];          // 编译器生成 LDR
REG->CTRL |= BIT;    // 往往 LDR + ORR + STR（或位带单 STR）
```

**MMIO 全书高潮在 Ch16** — 本章是 **指令层** 基础。

---

### 可复述一句话

> ARM **不能对内存直接运算** — Ch5 教你怎么 **把内存搬进寄存器、算完再写回**，以及 **地址怎么算**。
