# 02 · C 语言 · 系统级编程

**文件夹 `02`** · [LEARNING-CHAIN](../LEARNING-CHAIN.md)

> **定位：** **01 CSAPP 之后、03 Hennessy 之前** — 把「机器/程序长什么样」落成 **能写对的 C**。  
> **主线：** HFT / 内核 / MikanOS / DPDK 的 **共同语言**；**嵌入式 Linux 支线（19–24）同样绕不开 C** — 驱动、寄存器、用户态飞控逻辑都要用 C 直接和硬件、内核交互。  
> **09 C++** 是后续加 RAII，不是跳过 C。

---

## 为什么卡在这里？

| 上游 | 本模块 | 下游 |
|------|--------|------|
| [01 CSAPP](../01-CSAPP-3rd/) 硬件 + 机器级程序 **整体图景** | **指针、内存、系统调用思维** 写熟 | [03 Hennessy](../03-Computer-Architecture-6th/) → [04–07 内核/TLPI](../04-Linux-Kernel-Development/) → [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) |

**一句话：** CSAPP 建立硬件与程序图景；**本章把 C 写熟**；Hennessy 再量化 CPU/缓存；后面 OS/内核/嵌入式才能顺。

---

## 与体系结构同步练（可选）

读 **03 Hennessy** 时，可用 C 做小实验把理论落地 — 不必等嵌入式支线：

| 实验方向 | 练什么 | 对接 |
|----------|--------|------|
| x86-64 小程序 | 调用约定、栈帧、对齐 | CSAPP Ch3 + 本模块指针 |
| **ARM 裸机最小例**（QEMU） | EL 切换、异常向量、MMIO 写寄存器 | 预演 [19 ARM64](../19-ARM64-Architecture/) |
| 缓存/对齐微基准 | 结构体 padding、false sharing 直觉 | Hennessy Ch2 · 后接 HFT 热路径 |

**原则：** 主线仍是 K&R + *Pointers on C*；裸机/寄存器实验 **穿插在 02–03**，为后面 [21 驱动](../21-Linux-Device-Driver/) · [24 飞控](../24-Motion-Control-Motor/) 打手感，不另开 MCU 路线。

---

## 嵌入式支线 · C 是「通用母语」

| 场景 | 为何必须是 C |
|------|----------------|
| [19 ARM64](../19-ARM64-Architecture/) | 汇编与 C 互调、异常/特权级 |
| [21 驱动](../21-Linux-Device-Driver/) | 内核模块、寄存器、`ioremap` |
| [24 飞控](../24-Motion-Control-Motor/) | 用户态实时环、与驱动/ioctl 对接 |

**02 过关后**，嵌入式支线 **不必重学语法** — 直接复用指针、内存模型与 TLPI 思维（见 [HFT-READING-ROADMAP §六](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)）。

---

## 书目与笔记

📋 裁剪 → [OUTLINE.md](./OUTLINE.md)

| 优先级 | 书目 |
|--------|------|
| 🔴 | K&R · *Pointers on C* |
| 🟡 | 《嵌入式 C 语言自我修养》 |
| 🟡 | [01 CSAPP](../01-CSAPP-3rd/) Ch2–3（与 C 对照） |

---

## 与 09 C++ 的分工

**02 C** = 01 后立刻 · 系统级指针与内存  
**09 C++** = 08 OS / 07 TLPI 后 · muduo/HFT 引擎

→ [09-cpp-learning-notes](../09-cpp-learning-notes/) · [HFT 主次](../08-system-low-level-hands-on/HFT-AND-EMBEDDED-PRIORITY.md)

---

## 下一步

[03-Computer-Architecture-6th](../03-Computer-Architecture-6th/)
