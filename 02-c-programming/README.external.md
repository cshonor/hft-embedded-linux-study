# 上游 README 原文（归档）

> 来源：[cpp-learning-notes / 00-Linux-Kernel-DPDK-Network-C](https://github.com/cshonor/cpp-learning-notes/tree/main/00-Linux-Kernel-DPDK-Network-C)  
> 本仓入口以 [README.md](./README.md) 为准；下方为上游原文备份。

---

# 00 · C 语言（Linux 内核 / DPDK / 网络服务）

> 学习 C 的目的：读懂并编写 **Linux 内核**、**DPDK**、**Linux 网络服务** 相关代码。共 5 本书，内部分 `01–05`。  
> **HFT / 低延迟**：本目录排在 **01-C++Primer 之前**，从 **01 K&R** 开始即可，不必等 C++ 01–10。

## 为什么学 C

- Linux 内核、内核模块、网络协议栈以 C + GNU 扩展为主
- DPDK、高性能网卡旁路、用户态网络栈依赖 C 与底层内存模型
- 与 C++ 主线（01–10）配合：C++ 做业务与框架，C 啃内核与数据面

## 书单

| 阶段 | 目录 | 书籍 | 侧重 |
|------|------|------|------|
| 1 | [01-K-and-R-C](./01-K-and-R-C/) | 《C 程序设计语言（K&R 第2版）》 | 标准 C、`malloc`/指针/结构体 |
| 1 | [02-Pointers-on-C](./02-Pointers-on-C/) | 《C 和指针》 | 内存布局、联合体、ABI（读内核结构体基础） |
| 1 | [03-C-Traps-and-Pitfalls](./03-C-Traps-and-Pitfalls/) | 《C 陷阱与缺陷（第2版）》 | 宏、链接、库函数等常见陷阱 |
| 1 | [04-Expert-C-Programming](./04-Expert-C-Programming/) | 《C 专家编程》 | 链接器、深层指针规则、C 设计内幕 |
| 2 | [05-Embedded-C-Self-Cultivation](./05-Embedded-C-Self-Cultivation/) | 《嵌入式 C 语言自我修养》 | GNU-C 扩展（`__attribute__`、零长数组等），内核/DPDK 必读 |

## 学习顺序

### 默认（HFT / 数据面优先）

1. **阶段 1**：**01 → 02 → 03 → 04**（标准 C + 指针 + 陷阱 + 链接/内存）
2. **阶段 2**：**05**（GNU-C）→ Linux 网络 / **DPDK** / 内核网络源码

### 已有 C++ 基础时

可与阶段 1 并行扫 **03 陷阱**、**04 专家编程** 中链接与内存章节，加快上手内核/DPDK 源码。

## 学习进度

- [ ] 01 K&R
- [ ] 02 C 和指针
- [ ] 03 C 陷阱与缺陷
- [ ] 04 C 专家编程
- [ ] 05 嵌入式 C 语言自我修养

## 下一步

打开 **[01-K-and-R-C/ch01-introduction](./01-K-and-R-C/ch01-introduction/)** 开始第一本书。
