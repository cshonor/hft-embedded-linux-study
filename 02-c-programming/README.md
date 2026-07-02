# 02 · C 语言 · 系统级编程

**文件夹 `02`** · [LEARNING-CHAIN](../LEARNING-CHAIN.md) · [hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)

> **定位：** **01 CSAPP 之后、03 Hennessy 之前** — 把「机器/程序长什么样」落成 **能写对的 C**。  
> **笔记正文在外部仓：** [cpp-learning-notes / 11-Linux-Kernel-DPDK-Network-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C) — 本目录只负责 **HFT 读序、裁剪、与 01/03/19+ 的衔接**。  
> **09 C++** 是后续加 RAII，不是跳过 C。

---

## 笔记仓库（外部 · 在这里写）

| 入口 | 链接 |
|------|------|
| **C 语言专题** | [11-Linux-Kernel-DPDK-Network-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C) |
| 外部仓首页 | [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) |
| 本仓库裁剪 | [OUTLINE.md](./OUTLINE.md) |

**克隆（C/C++ 笔记共用一个外部仓）：**

```bash
git clone https://github.com/cshonor/cpp-learning-notes.git
```

> **编号说明：** 外部仓 C 专题是 **`11-…`**，本仓库主线是 **`02`** — **HFT 读序以本仓文件夹编号为准**（见 [OUTLINE](./OUTLINE.md) 对照表）。

---

## 为什么卡在这里？

| 上游 | 本模块 | 下游 |
|------|--------|------|
| [01 CSAPP](../01-CSAPP-3rd/) 硬件 + 机器级程序 **整体图景** | **指针、内存、GNU-C 思维** 写熟 | [03 Hennessy](../03-Computer-Architecture-6th/) → [04–07 内核/TLPI](../04-Linux-Kernel-Development/) → [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) |

**一句话：** CSAPP 建立硬件与程序图景；**本章把 C 写熟**；Hennessy 再量化 CPU/缓存；后面 OS/内核/嵌入式/DPDK 才能顺。

外部仓目标（与 HFT 一致）：读懂并编写 **Linux 内核**、**DPDK**、**网络数据面** 相关 C 代码 — 见 [外部 README](https://github.com/cshonor/cpp-learning-notes/blob/main/11-Linux-Kernel-DPDK-Network-C/README.md)。

---

## 外部 5 书 · HFT 读序速查

| 外部目录 | 书目 | 本链何时 |
|----------|------|----------|
| [01-K-and-R-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/01-K-and-R-C) | K&R | 🔴 **02 前半** · 与 CSAPP Ch2–3 对照 |
| [02-Pointers-on-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/02-Pointers-on-C) | *C 和指针* | 🔴 **02 核心** |
| [03-C-Traps-and-Pitfalls](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/03-C-Traps-and-Pitfalls) | *C 陷阱与缺陷* | 🟡 02 末或 03 Hennessy 并行 |
| [04-Expert-C-Programming](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming) | *C 专家编程* | 🟡 **开 04 LKD 前** · 链接/ABI |
| [05-Embedded-C-Self-Cultivation](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation) | 《嵌入式 C 自我修养》 | 🔴 **开 04 LKD 前** · GNU-C / 内核结构体 |

完整裁剪 → [OUTLINE.md](./OUTLINE.md)

---

## 与体系结构同步练（可选）

读 **03 Hennessy** 时，可用 C 做小实验把理论落地：

| 实验方向 | 练什么 | 对接 |
|----------|--------|------|
| x86-64 小程序 | 调用约定、栈帧、对齐 | CSAPP Ch3 + 外部 `01`/`02` |
| **ARM 裸机最小例**（QEMU） | EL 切换、异常向量、MMIO | 预演 [19 ARM64](../19-ARM64-Architecture/) |
| 缓存/对齐微基准 | 结构体 padding、false sharing | Hennessy Ch2 · 后接 HFT 热路径 |
| **字节序 + padding** | [endian_and_padding_demo.c](./code/endian_and_padding_demo.c) | CSAPP Ch2 · 一眼看清 x86 小端 |

---

## 嵌入式支线 · C 是「通用母语」

| 场景 | 为何必须是 C |
|------|----------------|
| [19 ARM64](../19-ARM64-Architecture/) | 汇编与 C 互调、异常/特权级 |
| [21 驱动](../21-Linux-Device-Driver/) | 内核模块、寄存器、`ioremap` |
| [24 飞控](../24-Motion-Control-Motor/) | 用户态实时环、与驱动/ioctl 对接 |

**02 过关后**，嵌入式支线 **不必重学语法** — 直接复用外部 `01`–`05` 笔记与 TLPI 思维。

---

## 与 09 C++ 的分工

| | **02 C（外部 `11`）** | **09 C++（外部 `01`–`10`）** |
|--|------------------------|------------------------------|
| 何时 | 01 CSAPP 后 · **03 Hennessy 前/并行** | 07 TLPI / 08 OS 后 |
| 角色 | 内核、DPDK、驱动、数据面 | muduo、HFT 引擎、业务框架 |

→ [09-cpp-learning-notes](../09-cpp-learning-notes/) · [HFT 主次](../08-system-low-level-hands-on/HFT-AND-EMBEDDED-PRIORITY.md)

---

## 下一步

[03-Computer-Architecture-6th](../03-Computer-Architecture-6th/)
