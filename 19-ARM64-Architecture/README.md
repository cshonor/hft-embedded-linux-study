# ARM64 架构 · 汇编前置

**文件夹 19** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** ARM **A 架构**（应用处理器）— **非** STM32 / MCU 裸机为主线。  
> **主线：** HFT（x86-64）；**本模块：** 飞行器 / 网关等 **嵌入式 Linux 退路**。  
> **双书并列：** **ARM32（Smith）** 与 **ARM64（奔跑吧）** 各自一个子目录。

---

## 目录结构（一眼分清）

```
19-ARM64-Architecture/
├── README.md                          ← 本页（模块总览）
├── arm32-smith-assembly/              ← Smith *ARM Assembly Language*（v4T / v7-M）
│   ├── OUTLINE.md
│   ├── chapter-01 … chapter-18
│   ├── appendix-* · glossary · references · code
│   └── _scripts/
└── arm64-programming-practice/        ← 《ARM64体系结构编程与实践》（AArch64）
    ├── OUTLINE.md
    └── chapter-01 … chapter-23
```

| 子目录 | 书 | 架构 | 入口 |
|--------|-----|------|------|
| **[arm32-smith-assembly/](./arm32-smith-assembly/)** | Smith *ARM Assembly Language* | **ARM32** · v4T / v7-M（ARM7 · Cortex-M） | [OUTLINE](./arm32-smith-assembly/OUTLINE.md) |
| **[arm64-programming-practice/](./arm64-programming-practice/)** | 奔跑吧 *ARM64体系结构编程与实践* | **AArch64** · ARMv8/v9 | [OUTLINE](./arm64-programming-practice/OUTLINE.md) |

---

## 必读书（2 本 · 分工明确）

| # | 书目 | 读什么 | 放哪 |
|---|------|--------|------|
| 1 | ***ARM Assembly Language*** — William Sw Smith | **v4T / v7-M 汇编思维** — Load/Store · 栈 · MMIO · C/汇编互调 | [**arm32-smith-assembly/**](./arm32-smith-assembly/) |
| 2 | **《ARM64体系结构编程与实践》** — 奔跑吧 | **A64 · 异常/GIC · 内存管理** · 树莓派 4B / **QEMU** | [**arm64-programming-practice/**](./arm64-programming-practice/) |

**架构边界：** Smith **不是 AArch64 教材**；**AArch64 主战场在奔跑吧**。

**推荐顺序：**

```
Smith Ch2–8、13、16、18（可选/压缩，在 arm32-smith-assembly/）
        ↓
奔跑吧 Ch1–23（AArch64 主书，在 arm64-programming-practice/）
```

已有 x86/CSAPP 汇编直觉者，可 **跳过 Smith，直接从奔跑吧 Ch1**。

---

## 为何汇编前置

| 后续模块 | 需要汇编直觉 |
|----------|--------------|
| [20 构建](../20-UBoot-Kernel-Build/) | U-Boot 启动早期、内核 `head.S` |
| [21 驱动](../21-Linux-Device-Driver/) | 中断入口、原子指令 `ldxr`/`stxr` |
| [04 LKD](../04-Linux-Kernel-Development/) | 对照 x86 `syscall` ↔ ARM `svc` |

**顺序：** Smith 精读章（或跳过）→ [**奔跑吧 OUTLINE**](./arm64-programming-practice/OUTLINE.md) → [20 Mastering Embedded Linux Programming](../20-UBoot-Kernel-Build/)。

---

## 《ARM64体系结构编程与实践》（摘要）

📋 全文裁剪 → [arm64-programming-practice/OUTLINE.md](./arm64-programming-practice/OUTLINE.md) · [导读](./arm64-programming-practice/README.md)

**实验代码：** [runninglinuxkernel/arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)

---

## *ARM Assembly Language*（Smith · 摘要）

📋 全文裁剪 → [arm32-smith-assembly/OUTLINE.md](./arm32-smith-assembly/OUTLINE.md) · [导读](./arm32-smith-assembly/README.md)

精读建议：**Ch2–5、7–8、13、16、18**（浮点 Ch9–11 可跳）。

---

## 复用（HFT 链）

| 已有 | 本模块用法 |
|------|------------|
| **[02 C](../02-c-programming/)** | 汇编与 C 互调、指针/MMIO |
| [01 CSAPP](../01-CSAPP-3rd/) x86-64 | **对照学** cache、调用约定 |
| [03 Hennessy](../03-Computer-Architecture-6th/) Ch2 | MESI — ARM 同样适用 |
| [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) | UEFI/x86 启动链 — 与 ARM **概念平行** |

---

## x86 ↔ ARM64 对照要点

| 主题 | x86-64（已学） | ARM64（奔跑吧） |
|------|----------------|-----------------|
| 特权级 | Ring 0–3 | **EL0–EL3** |
| 系统调用 | `syscall` | **`svc`** |
| 原子/屏障 | `lock` / `mfence` | **`ldxr`/`stxr` · DMB/DSB** |

---

## 验收

- [ ] 完成 **奔跑吧** 精读章（A64 · 异常/GIC/MM · 屏障/原子）  
- [ ] （可选）Smith 精读章 — Load/Store · 栈 · MMIO · C/汇编  
- [ ] 能解释 **EL1 / EL0** 与 x86 Ring 的对应  
- [ ] QEMU ARM64 或树莓派上跑通至少 1 个实验  
- [ ] 知道设备树为何取代 hard-coded 寄存器（→ [22](../22-Device-Tree-Study/)）

**下一章：** [20 嵌入式 Linux 构建](../20-UBoot-Kernel-Build/)
