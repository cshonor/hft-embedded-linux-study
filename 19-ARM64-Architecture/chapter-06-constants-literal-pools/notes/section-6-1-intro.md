## §6.1 简介

> **Ch 6 · 常量与文字池** · [章导读](../README.md)

---

### 本章在全书中的位置

| | |
|---|---|
| **角色** | **选读** — 理解 **`LDR rd,=imm`** 背后机制；MMIO 基址/掩码天天用 |
| **核心矛盾** | ARM **32 bit 指令** 无法塞进完整 **32 bit 立即数** |
| **前置** | [Ch3 §3.5 `LDR =`](../../chapter-03-instruction-sets-v4t-v7m/notes/section-3-5-example-register-swap.md) · [Ch4 LTORG](../../chapter-04-assembler-rules-directives/notes/section-4-4-directives.md) · [Ch5 PC 相对 LDR](../../chapter-05-loads-stores-addressing/notes/section-5-3-load-store.md) |

---

### 四主题骨架

```
§6.2  循环移位立即数 — 8 bit + 偶数 ROR · MVN 扩展
§6.3  LDR= 伪指令 — 汇编器选 MOV/MVN 或走文字池
§6.4  文字池 · LTORG · MOVW/MOVT（Thumb-2 直载）
§6.5  地址：ADR / ADRL / LDR=label
```

---

### 阅读策略（嵌入式 Linux 支线）

| 块 | 建议 |
|----|------|
| **§6.2–6.4 概念** | **速读** — 知道 listing 里为何突然出现 `LDR [pc,#n]` |
| **§6.3 `LDR =`** | **必记用法** — 与 Ch5 一体 |
| **MOVW/MOVT** | M4/Thumb-2 常用；AArch64 见 `movz`/`movk` |
| **§6.5 地址** | 读启动代码/字符串表时用到 |

---

### 可复述一句话

> Ch6 回答：**32 位常数/地址怎么进寄存器** — 能编码进指令就 `MOV`，否则 **文字池 + PC 相对 LDR**，或 **MOVW/MOVT 拼出来**。
