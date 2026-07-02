# C 语言 · HFT 主线裁剪 OUTLINE

> **读序：** `01 CSAPP` → **02 C（本模块）** → `03 Hennessy` → `04–07` 内核/TLPI → `08/01` MikanOS  
> **笔记正文：** [外部 11-Linux-Kernel-DPDK-Network-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C)（**HFT 读序 ≠ 外部仓 `11` 文件夹编号**）

---

## 本仓 `02` ↔ 外部仓目录

| 本链阶段 | 外部目录 | 书目 |
|----------|----------|------|
| 🔴 02 必过 | [01-K-and-R-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/01-K-and-R-C) | K&R |
| 🔴 02 必过 | [02-Pointers-on-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/02-Pointers-on-C) | *C 和指针* |
| 🟡 02 末 / 03 并行 | [03-C-Traps-and-Pitfalls](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/03-C-Traps-and-Pitfalls) | *C 陷阱与缺陷* |
| 🟡 **04 LKD 前** | [04-Expert-C-Programming](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming) | *C 专家编程* |
| 🔴 **04 LKD 前** | [05-Embedded-C-Self-Cultivation](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation) | 《嵌入式 C 自我修养》 |

外部仓建议顺序：**01 → 02 → 03 → 04**（阶段 1）→ **05**（阶段 2，GNU-C）→ 再开 LKD / 内核网 / DPDK。

---

## 🔴 必做（开 03 Hennessy 前至少完成）

| 来源 | 内容 | HFT 为何读 |
|------|------|------------|
| **外部 `01` K&R** | Ch1–5、8 | 标准 C、指针、结构体 |
| **外部 `02` Pointers on C** | 核心章 | 内存布局、ABI — **读内核结构体基础** |
| **01 CSAPP** | Ch2、Ch3、Ch5 导论 | 与 C **对照**，不另开纯语法课 |

**验收：** 能写无 UB 的指针操作、解释结构体对齐、读懂简单 `malloc`/栈布局。

---

## 🟡 选读 / 可后移（但 LKD 前建议补完）

| 来源 | 何时 |
|------|------|
| **外部 `03` C 陷阱与缺陷** | 宏、链接、库函数陷阱 — 02 末或 03 并行 |
| **外部 `04` C 专家编程** | 链接器、深层指针 — **04 LKD 前** |
| **外部 `05` 嵌入式 C 自我修养** | `__attribute__`、零长数组 — **04 LKD / 14 DPDK 前必读** |
| **K&R** Ch6–7 | 与 07 TLPI I/O 对照 |

---

## 🟢 同步实践（02 学 C · 03 学 Hennessy 时穿插）

| 练习 | 目的 |
|------|------|
| CSAPP Lab / 自写小程序 | 指针、内存布局、UB 边界 | [01/code/ch02-demo](../01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c) · [02/code/pointer-and-bytes.c](./code/pointer-and-bytes.c) |
| QEMU **ARM 裸机 hello + 异常**（可选） | CPU 模式/异常向量 — 预演 19 ARM64 |
| 结构体对齐 / cache line 微测 | 对接 Hennessy Ch2 · 后接 HFT 伪共享 |

---

## 阶段衔接

```text
01 CSAPP → 02 C（外部 01–02 必过；05 在 04 LKD 前）
    → 03 Hennessy → 04–07 内核/TLPI
    → 08/01 MikanOS → 09 C++ → 10 PNP → … → 14 DPDK → 17 HFT
```

---

## 学习进度（在外部仓 README 打勾）

同步外部 checklist → [11-Linux-Kernel-DPDK-Network-C/README.md](https://github.com/cshonor/cpp-learning-notes/blob/main/11-Linux-Kernel-DPDK-Network-C/README.md)

← [README](./README.md) · [LEARNING-CHAIN](../LEARNING-CHAIN.md) · [09 C++ 索引](../09-cpp-learning-notes/)
