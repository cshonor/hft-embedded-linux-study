## Ch6 完整概述 · 常量与文字池

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Constants and Literal Pools · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **32 bit 立即数限制** | 指令字放不下完整常数 — ARM 用旋转 imm8、文字池、MOVW/MOVT |
| **伪指令本质** | **`LDR =`** / **`ADR`** 由汇编器展开 — 读 listing 必备 |
| **实战** | MMIO 基址、掩码、表地址 — 与 **Ch5 Load/Store** 衔接 |

**前置：** [Ch5 Load/Store](../chapter-05-loads-stores-addressing/notes/section-0-本章完整概述.md) · [Ch4 LTORG](../chapter-04-assembler-rules-directives/notes/section-4-4-directives.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **循环移位立即数** | §6.2 | [section-6-2-rotate-constants.md](./section-6-2-rotate-constants.md) |
| **`LDR =` 伪指令** | §6.3 | [section-6-3-load-constants.md](./section-6-3-load-constants.md) |
| **文字池 · LTORG · MOVW/MOVT** | §6.4 | [section-6-4-literal-pools.md](./section-6-4-literal-pools.md) |
| **加载地址 ADR/ADRL** | §6.5 | [section-6-5-load-addresses.md](./section-6-5-load-addresses.md) |

---

### 三、知识流（口述版）

```
32 bit 指令 → 最多 12 bit 编码立即数（imm8 ROR 偶数位）
        ↓
MOV/MVN 能表示？ → 单条指令
        ↓ 否
文字池 DCD 常数 + LDR [pc,#off] ；太长则 LTORG 分段
        ↓
或 MOVW/MOVT 两条拼 32 bit（Thumb-2）
        ↓
地址：近 → ADR ；远/外部 → LDR=label
        ↓
Ch7：寄存器里的常数参与 ALU
```

---

### 四、与支线对照

| 场景 | Ch6 写法 |
|------|----------|
| UART 基址 | `LDR r1,=0x40001000` 或 MOVW/MOVT |
| 位掩码 | `LDR r2,=BIT_MASK` — 汇编器优化 MOV |
| 字符串 | `ADR r0, msg` |
| AArch64 内核 | `movz`/`movk` · `adrp`+`add` |

---

### 五、下一章

→ **[Ch7 整数逻辑与算术](../../chapter-07-integer-logic-arithmetic/)**（**精读**）
