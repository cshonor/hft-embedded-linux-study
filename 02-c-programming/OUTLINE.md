# C 语言 · HFT 主线裁剪 OUTLINE

> **读序：** `01 CSAPP` → **02 C（本模块）** → `03 Hennessy` → `04–07` 内核/TLPI → `08/01` MikanOS

---

## 🔴 必做

| 来源 | 内容 | HFT 为何读 |
|------|------|------------|
| **K&R** | Ch1–5、8 | 语法 + 指针 + 函数 |
| **Pointers on C** | 核心章 | 内存、指针 — **LKD/TLPI 前置** |
| **01 CSAPP** | Ch2、Ch3、Ch5 导论 | 与 C **对照**，不另开纯语法课 |

---

## 🟡 选读

| 来源 | 何时 |
|------|------|
| **嵌入式 C 自我修养** | 读 LKD / MikanOS / **21 驱动** 前补链接与 ELF |
| **K&R** Ch6–7 | 与 TLPI I/O 对照 |

---

## 🟢 同步实践（02 学 C · 03 学 Hennessy 时穿插）

| 练习 | 目的 |
|------|------|
| CSAPP Lab / 自写小程序 | 指针、内存布局、UB 边界 |
| QEMU **ARM 裸机 hello + 异常**（可选） | CPU 模式/异常向量 — 预演 19 ARM64 |
| 结构体对齐 / cache line 微测 | 对接 Hennessy Ch2 · 后接 HFT 伪共享 |

> 嵌入式 Linux 的驱动与飞控 **底层全是 C**；此处练的是 **和硬件对话的手感**，不是另开 STM32 路线。

---

## 阶段衔接

```text
01 CSAPP → 02 C → 03 Hennessy → 04–07 内核/TLPI
    → 08/01 MikanOS → 09 C++ → 10 PNP → … → 17 HFT
```

← [README](./README.md) · [LEARNING-CHAIN](../LEARNING-CHAIN.md)
