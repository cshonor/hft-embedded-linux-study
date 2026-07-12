# ARM64 架构 · 汇编前置

**文件夹 19** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** ARM **A 架构**（应用处理器）— **非** STM32 / MCU 裸机。  
> **主线：** HFT（x86-64）；**本模块：** 飞行器 / 网关等 **嵌入式 Linux 退路**。  
> **双书模型：** Smith = 汇编思维入门（v4T/v7-M，可选/压缩）；**《ARM64体系结构编程与实践》= AArch64 实战主书**。

---

## 必读书（2 本 · 分工明确）

| # | 书目 | 读什么 | 索引 |
|---|------|--------|------|
| 1 | ***ARM Assembly Language*** — William Sw Smith | **v4T / v7-M 汇编思维** — Load/Store · 栈 · MMIO · C/汇编互调（→ [OUTLINE.md](./OUTLINE.md)） | 本目录 `chapter-*` |
| 2 | **《ARM64体系结构编程与实践》** — 奔跑吧Linux社区 · 人民邮电 | **ARMv8/v9 · A64 64 位** · 异常/中断 · **GIC** · **内存管理** · 树莓派 4B / **QEMU** 实验 | [**arm64-programming-practice/**](./arm64-programming-practice/) |

**架构边界：** Smith **不是 AArch64 教材** — 正文为 **ARM7TDMI / Cortex-M**；**AArch64 主战场在奔跑吧一书**（替代原先可选的 *ARMv8-A Programmer's Guide* 作为实战路径）。

**推荐顺序（无人机 / 高端嵌入式）：**

```
Smith Ch2–8、13、16、18（可选/压缩）  →  奔跑吧 Ch1–23（AArch64 主书）
```

已有 x86/CSAPP 汇编直觉者，可 **跳过 Smith，直接从奔跑吧 Ch1 开 AArch64**。

---

## 为何汇编前置

| 后续模块 | 需要汇编直觉 |
|----------|--------------|
| [20 构建](../20-UBoot-Kernel-Build/) | U-Boot 启动早期、内核 `head.S` |
| [21 驱动](../21-Linux-Device-Driver/) | 中断入口、原子指令 `ldxr`/`stxr` |
| [04 LKD](../04-Linux-Kernel-Development/) | 对照 x86 `syscall` ↔ ARM `svc` |

**顺序：** Smith 精读章（或跳过）→ [**奔跑吧 OUTLINE**](./arm64-programming-practice/OUTLINE.md) Ch1–14、18–21 → 再开 [20 Mastering Embedded Linux Programming](../20-UBoot-Kernel-Build/)。

---

## 章节目录（文件夹）

### 《ARM64体系结构编程与实践》（AArch64 主书 · Ch 1–23）

📋 **阅读裁剪与标签** → [arm64-programming-practice/OUTLINE.md](./arm64-programming-practice/OUTLINE.md)

| 块 | 文件夹 | 标签 |
|----|--------|------|
| 架构 + 环境 | [ch01](./arm64-programming-practice/chapter-01-arm64-fundamentals/) · [ch02](./arm64-programming-practice/chapter-02-raspberry-pi-lab/) | **精读** |
| A64 指令集 | [ch03](./arm64-programming-practice/chapter-03-a64-load-store/) … [ch07](./arm64-programming-practice/chapter-07-a64-traps/) | **精读** |
| 工具链 | [ch08](./arm64-programming-practice/chapter-08-gnu-assembler/) … [ch10](./arm64-programming-practice/chapter-10-gcc-inline-asm/) | **精读** |
| 异常/GIC/MM | [ch11](./arm64-programming-practice/chapter-11-exception-handling/) … [ch14](./arm64-programming-practice/chapter-14-memory-management/) | **精读** |
| 屏障/原子 | [ch18](./arm64-programming-practice/chapter-18-memory-barriers/) … [ch20](./arm64-programming-practice/chapter-20-atomic-operations/) | **精读** |
| 全书索引 | [arm64-programming-practice/](./arm64-programming-practice/) | — |

**实验代码：** [runninglinuxkernel/arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)

---

### *ARM Assembly Language*（Smith · v4T/v7-M · Ch 1–18）

📋 **阅读裁剪与标签** → [OUTLINE.md](./OUTLINE.md)

| 章 | 文件夹 | 标签 |
|----|--------|------|
| 1 | [chapter-01-overview-computing-systems](./chapter-01-overview-computing-systems/) | 选读 |
| 2 | [chapter-02-programmers-model](./chapter-02-programmers-model/) | **精读** |
| 3 | [chapter-03-instruction-sets-v4t-v7m](./chapter-03-instruction-sets-v4t-v7m/) | **精读** |
| 4 | [chapter-04-assembler-rules-directives](./chapter-04-assembler-rules-directives/) | **精读** |
| 5 | [chapter-05-loads-stores-addressing](./chapter-05-loads-stores-addressing/) | **精读** |
| 6 | [chapter-06-constants-literal-pools](./chapter-06-constants-literal-pools/) | 选读 |
| 7 | [chapter-07-integer-logic-arithmetic](./chapter-07-integer-logic-arithmetic/) | **精读** |
| 8 | [chapter-08-branches-loops](./chapter-08-branches-loops/) | **精读** |
| 9–11 | [ch09](./chapter-09-floating-point-basics/) · [ch10](./chapter-10-floating-point-rounding-exceptions/) · [ch11](./chapter-11-floating-point-data-processing/) | 跳过 |
| 12 | [chapter-12-tables](./chapter-12-tables/) | 选读 |
| 13 | [chapter-13-subroutines-stacks](./chapter-13-subroutines-stacks/) | **精读** |
| 14–15 | [ch14](./chapter-14-exception-handling-arm7tdmi/) · [ch15](./chapter-15-exception-handling-v7m/) | 选读 |
| 16 | [chapter-16-memory-mapped-peripherals](./chapter-16-memory-mapped-peripherals/) | **精读** |
| 17 | [chapter-17-arm-thumb-thumb2-instructions](./chapter-17-arm-thumb-thumb2-instructions/) | 选读 |
| 18 | [chapter-18-mixing-c-and-assembly](./chapter-18-mixing-c-and-assembly/) | **精读** |

**附录与其他：** [appendix A–D](./appendix-A-code-composer-studio/) · [glossary/](./glossary/) · [references/](./references/) · [code/](./code/)

---

## 复用（HFT 链）

| 已有 | 本模块用法 |
|------|------------|
| **[02 C](../02-c-programming/)** | 汇编与 C 互调、指针/MMIO |
| [01 CSAPP](../01-CSAPP-3rd/) x86-64 | **对照学** cache、调用约定 |
| [03 Hennessy](../03-Computer-Architecture-6th/) Ch2 | MESI — ARM 同样适用 |
| [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) Ch3+ | UEFI/x86 启动链 — 与 ARM **概念平行**（Loader → 内核） |

---

## x86 ↔ ARM64 对照要点

| 主题 | x86-64（已学） | ARM64（本章） |
|------|----------------|---------------|
| 特权级 | Ring 0–3 | **EL0–EL3** |
| 系统调用 | `syscall` | **`svc`** |
| 页表 | 4/5 级 | **4 级**（类似） |
| 原子/屏障 | `lock` / `mfence` | **`ldxr`/`stxr` · DMB/DSB** |

---

## 验收

- [ ] 完成 **奔跑吧** [OUTLINE](./arm64-programming-practice/OUTLINE.md) 精读章（A64 · 异常/GIC/MM · 屏障/原子）  
- [ ] （可选）Smith 精读章 — Load/Store · 栈 · MMIO · C/汇编  
- [ ] 能解释 **EL1 内核 / EL0 用户态** 与 x86 Ring 的对应关系  
- [ ] 在 **QEMU ARM64** 或树莓派 4B 上跑通至少 1 个配套实验  
- [ ] 知道 **设备树** 为何取代 hard-coded 寄存器（→ [22](../22-Device-Tree-Study/)）

**下一章：** [20 嵌入式 Linux 构建](../20-UBoot-Kernel-Build/)
