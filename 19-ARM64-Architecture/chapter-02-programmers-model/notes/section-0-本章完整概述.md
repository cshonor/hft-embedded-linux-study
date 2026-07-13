## Ch2 完整概述 · 程序员模型

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** The Programmer's Model · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **程序员视角的 CPU** | 不画流水线，只建立 **寄存器、模式、异常入口、数据宽度** 心智模型 |
| **双线架构** | **ARM7TDMI (v4T)** 经典 vs **Cortex-M4 (v7-M)** 本书主战场 |
| **Ch3 前置** | 读 listing / 写第一条 `MOV` 前，必须知道 **r13–r15、CPSR/xPSR、向量表** |

**前置：** [Ch1 完整概述](../../chapter-01-overview-computing-systems/notes/section-0-本章完整概述.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **数据类型、对齐、s/u、f32/f64 映射** | §2.2 | [section-2-2-data-types.md](./section-2-2-data-types.md) |
| **ARM7TDMI 模型** | §2.3 | [section-2-3-arm7tdmi.md](./section-2-3-arm7tdmi.md) |
| **Cortex-M4 模型** | §2.4 | [section-2-4-cortex-m4.md](./section-2-4-cortex-m4.md) |

---

### 三、知识流（口述版）

```
32 bit 自然字长 · s8/u8…s32/u32 → byte/half/word
        ↓
对齐：访问宽度决定 · f32=word · f64=8B（算在 Ch9）
        ↓
ARM7：7 模式 · banked SP/LR · 向量表 = B 指令
        ↓
M4：Thread/Handler · MSP/PSP · s0–s31（M4F）
        ↓
Ch3：在这些寄存器上跑第一个示例程序
```

---

### 四、架构对照速查

| | ARM7TDMI | Cortex-M4 | AArch64（[奔跑吧](../../arm64-programming-practice/)） |
|---|----------|-----------|--------------------------------------------------------|
| 特权 | 7 模式 | Thread/Handler + Priv | **EL0–EL3** |
| 栈指针 | 按模式 bank | MSP / PSP | **SP_ELx** |
| 向量表 | 跳转指令 | 地址表 | **VBAR** + 异常向量 |
| 本书权重 | 概念/对照 | **实验主平台** | 嵌入式 Linux 主书 |

---

### 五、与 HFT / 嵌入式支线

| 路径 | Ch2 呼应 |
|------|----------|
| [08 MikanOS](../../../08-system-low-level-hands-on/01-mikan-os/) x86 | 同样区分 **用户/内核栈、中断向量、PC** |
| [04 LKD](../../../04-Linux-Kernel-Development/) | 内核 `entry.S` 保存寄存器帧 ≈ M4 硬件压栈 + 软件扩展 |
| [21 驱动](../../../21-Linux-Device-Driver/) | 中断上下文 **不能睡眠** — 从 Handler mode 直觉理解 |
| [20 构建](../../../20-UBoot-Kernel-Build/) | U-Boot 启动时设 **SP、跳 Reset** — 同向量表第一项 |

---

### 六、下一章

→ **[Ch3 指令集简介](../../chapter-03-instruction-sets-v4t-v7m/)** — ARM/Thumb 对比 + 5 个寄存器级示例（**精读**）
