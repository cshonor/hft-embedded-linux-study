## §8.1 简介

> **Ch 8 · 分支与循环** · [章导读](../README.md)

---

### 本章在全书中的位置

| | |
|---|---|
| **角色** | **精读** — 把 **Ch7 标志位** 变成 **控制流**；子程序 **`BL`** 预告 **Ch13** |
| **核心矛盾** | 流水线 CPU 上 **分支 = 清空预取** → 延迟；ARM 用 **条件执行 / IT / 展开** 缓解 |
| **前置** | [Ch7 标志/CMP](../../chapter-07-integer-logic-arithmetic/notes/section-0-本章完整概述.md) · [Ch3 阶乘 IT](../../chapter-03-instruction-sets-v4t-v7m/notes/section-3-4-example-factorial.md) |

---

### 五主题骨架

```
§8.2  B · BX · BL · BLX · CBZ/CBNZ · 条件分支
§8.3  While · For（向下计数）· Do-While
§8.4  ARM 条件后缀 vs M4 IT 块
§8.5  循环展开 — 空间换时间、确定性周期
```

---

### 与 HFT / 嵌入式

| 场景 | Ch8 呼应 |
|------|----------|
| 热路径少分支 | 条件执行、展开 — 同 [16 HFT](../../../16-HFT-Low-Latency/) |
| 驱动 poll 循环 | `SUBS`+`BNE` 向下计数 |
| 内核 spin_wait | 展开或 `CBNZ` 短跳 |

---

### 可复述一句话

> Ch8 = **怎么跳、怎么循环、怎么少跳** — 分支是必需品，也是流水线代价来源。
