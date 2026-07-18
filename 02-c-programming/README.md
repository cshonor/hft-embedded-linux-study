# 02 · C 语言 · 系统级编程

**文件夹 `02`** · [LEARNING-CHAIN](../LEARNING-CHAIN.md) · [OUTLINE](./OUTLINE.md)

> **定位：** **01 CSAPP 之后、03 Hennessy 之前** — 把「机器/程序长什么样」落成 **能写对的 C**。  
> **笔记正文已在本目录：** 自 [cpp-learning-notes / 00-Linux-Kernel-DPDK-Network-C](https://github.com/cshonor/cpp-learning-notes/tree/main/00-Linux-Kernel-DPDK-Network-C) 复制的 **01–05** 五书。  
> **09 C++** 是后续加 RAII，不是跳过 C。

---

## 为什么学 C

- Linux 内核、内核模块、网络协议栈以 C + GNU 扩展为主
- DPDK、高性能网卡旁路、用户态网络栈依赖 C 与底层内存模型
- 与 C++ 主线配合：C++ 做业务与框架，C 啃内核与数据面

---

## 书单（本目录）

| 阶段 | 目录 | 书籍 | 侧重 |
|------|------|------|------|
| 1 | [01-K-and-R-C](./01-K-and-R-C/) | 《C 程序设计语言（K&R 第2版）》 | 标准 C、`malloc`/指针/结构体 |
| 1 | [02-Pointers-on-C](./02-Pointers-on-C/) | 《C 和指针》 | 内存布局、联合体、ABI |
| 1 | [03-C-Traps-and-Pitfalls](./03-C-Traps-and-Pitfalls/) | 《C 陷阱与缺陷（第2版）》 | 宏、链接、库函数陷阱 |
| 1 | [04-Expert-C-Programming](./04-Expert-C-Programming/) | 《C 专家编程》 | 链接器、深层指针、C 设计内幕 |
| 2 | [05-Embedded-C-Self-Cultivation](./05-Embedded-C-Self-Cultivation/) | 《嵌入式 C 语言自我修养》 | GNU-C（`__attribute__`、零长数组等） |

来源副本说明 → [README.external.md](./README.external.md)（上游原文）

> **CSAPP 实验在 [01-CSAPP-3rd/code](../01-CSAPP-3rd/code/)**（ABI、endian、指针步长）— **不在 02 重复**。

---

## 学习顺序

### 默认（HFT / 数据面优先）

1. **阶段 1：** **01 → 02 → 03 → 04**
2. **阶段 2：** **05**（GNU-C）→ [04 LKD](../04-Linux-Kernel-Development/) / [14 DPDK](../14-DPDK-Low-Latency-Network/) / 内核网络

完整裁剪与验收 → [OUTLINE.md](./OUTLINE.md)

### 学习进度

- [ ] 01 K&R
- [ ] 02 C 和指针
- [ ] 03 C 陷阱与缺陷
- [ ] 04 C 专家编程
- [ ] 05 嵌入式 C 语言自我修养

---

## 在主线中的位置

| 上游 | 本模块 | 下游 |
|------|--------|------|
| [01 CSAPP](../01-CSAPP-3rd/) | **指针、内存、GNU-C** | [03 Hennessy](../03-Computer-Architecture-6th/) → [04–07](../04-Linux-Kernel-Development/) → [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) |

**下一步：** 打开 **[01-K-and-R-C/ch01-introduction](./01-K-and-R-C/ch01-introduction/)**，或学完后进 [03-Computer-Architecture-6th](../03-Computer-Architecture-6th/)。
