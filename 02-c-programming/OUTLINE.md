# C 语言 · HFT 主线裁剪 OUTLINE

> **读序：** `01 CSAPP` → **02 C（本模块）** → `03 Hennessy` → `04–07` 内核/TLPI → `08/01` MikanOS  
> **笔记正文：** 本目录 `01`–`05`（自 [上游 00-Linux-Kernel-DPDK-Network-C](https://github.com/cshonor/cpp-learning-notes/tree/main/00-Linux-Kernel-DPDK-Network-C) 复制）

---

## 本仓 `02` 书目录

| 本链阶段 | 目录 | 书目 |
|----------|------|------|
| 🔴 02 必过 | [01-K-and-R-C](./01-K-and-R-C/) | K&R |
| 🔴 02 必过 | [02-Pointers-on-C](./02-Pointers-on-C/) | *C 和指针* |
| 🟡 02 末 / 03 并行 | [03-C-Traps-and-Pitfalls](./03-C-Traps-and-Pitfalls/) | *C 陷阱与缺陷* |
| 🟡 **04 LKD 前** | [04-Expert-C-Programming](./04-Expert-C-Programming/) | *C 专家编程* |
| 🔴 **04 LKD 前** | [05-Embedded-C-Self-Cultivation](./05-Embedded-C-Self-Cultivation/) | 《嵌入式 C 自我修养》 |

建议顺序：**01 → 02 → 03 → 04**（阶段 1）→ **05**（阶段 2，GNU-C）→ 再开 LKD / 内核网 / DPDK。

---

## 🔴 必做（开 03 Hennessy 前至少完成）

| 来源 | 内容 | HFT 为何读 |
|------|------|------------|
| **`01` K&R** | Ch1–5、8 | 标准 C、指针、结构体 |
| **`02` Pointers on C** | 核心章 | 内存布局、ABI — **读内核结构体基础** |
| **01 CSAPP** | Ch2、Ch3、Ch5 导论 | 与 C **对照**，不另开纯语法课 |

**验收：** 能写无 UB 的指针操作、解释结构体对齐、读懂简单 `malloc`/栈布局；能说出 **API vs ABI** → [01 CSAPP · ABI 笔记](../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md)

---

## 🟡 选读 / 可后移（但 LKD 前建议补完）

| 来源 | 何时 |
|------|------|
| **`03` C 陷阱与缺陷** | 宏、链接、库函数陷阱 — 02 末或 03 并行 |
| **`04` C 专家编程** | 链接器、深层指针 — **04 LKD 前** |
| **`05` 嵌入式 C 自我修养** | `__attribute__`、零长数组 — **04 LKD / 14 DPDK 前必读** |
| **K&R** Ch6–7 | 与 07 TLPI I/O 对照 |

---

## 🟢 同步实践（02 学 C · 03 学 Hennessy 时穿插）

| 练习 | 目的 |
|------|------|
| CSAPP 实验 | 01 [code/](../01-CSAPP-3rd/code/) · [ABI](../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md) · [指针步长](../01-CSAPP-3rd/chapter-03-machine-level-programs/notes/section-3.8-指针步长详解.md) |
| QEMU **ARM 裸机 hello + 异常**（可选） | CPU 模式/异常向量 — 预演 19 ARM64 |
| 结构体对齐 / cache line 微测 | 对接 Hennessy Ch2 · 后接 HFT 伪共享 |

---

## 阶段衔接

```text
01 CSAPP → 02 C（本目录 01–02 必过；05 在 04 LKD 前）
    → 03 Hennessy → 04–07 内核/TLPI
    → 08/01 MikanOS → 09 C++ → … → 14 DPDK → 17 HFT
```

---

## 学习进度

在 [README.md](./README.md) 勾选；上游原文见 [README.external.md](./README.external.md)。

← [README](./README.md) · [LEARNING-CHAIN](../LEARNING-CHAIN.md)
