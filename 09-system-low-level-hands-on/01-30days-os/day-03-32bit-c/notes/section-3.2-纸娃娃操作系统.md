## ② 纸娃娃操作系统 · 画面模式切换

> 软盘第 1 扇区 / 第 2 扇区起 / Day 2 的 `0x00` 占位 — 先看 [Day 3 README 最直白版](../README.md#软盘上到底长什么样最直白版)。

系统正式命名：**纸娃娃操作系统（haribote-os / はりぼて）** — 目前 **只有外壳**，但按步骤能 **长成完整 OS**。

#### 如何确认 OS 本体已执行？

用 **显卡 BIOS `INT 0x10`** 切换到 **VGA 模式 0x13**（320×200、256 色索引）：

| 模式 | 参数 | 现象 |
|------|------|------|
| **VGA 图形 0x13** | **`mov ax, 0x0013`** + **`int 0x10`** | 启动后 **整屏全黑** = **OS 已跑起来** |

（Day 1 的 **`load done`** 是 **文本模式 `0xB8000`**；**0x13 图形** 证明 **控制权已到 bootpack** 且 **nasmhead 已切显存映射**。）

| 小节 | 内容 |
|------|------|
| [§3.2.1 VGA 模式 0x13 详解](./section-3.2.1-VGA模式0x13详解.md) | 寻址公式、**0x03 vs 0x13**、切换汇编、为何原书选此模式 |

代码：切模式在 [nasmhead.asm](../code/sec-3.4-bootpack-asm-and-c/nasmhead.asm) · 说明 [sec-3.2-vga-mode-0x13](../code/sec-3.2-vga-mode-0x13/) · 填黑在 [bootpack.c](../code/sec-3.4-bootpack-asm-and-c/bootpack.c)

---
