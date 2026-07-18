# 上游 README 原文（归档）

> 来源：[cshonor/cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)  
> 本仓入口以 [README.md](./README.md) 为准；下方为上游原文备份（路径按上游根目录）。

---

# 学习笔记目录

按推荐阅读顺序整理，笔记与代码对应根目录下各书籍子目录。

## 00 · C 语言 / HFT 数据面（优先）

面向 **DPDK、内核网络、用户态数据面** —— **先啃 C，再按需补 C++**。目录排在 **01-C++Primer 之前**。

| 入口 | 说明 |
|------|------|
| **[00-Linux-Kernel-DPDK-Network-C](./00-Linux-Kernel-DPDK-Network-C/)** | 5 本 C（内部分 `01–05`），HFT 数据面核心 |
| [根目录 HFT 路线说明](#hft--低延迟推荐顺序) | 阶段 A/B/C 总览 |

```text
阶段 A  C 基础（00 内 01→05）
  01 K&R → 02 C和指针 → 03 陷阱与缺陷 → 04 专家编程 → 05 GNU-C
        ↓
阶段 B  系统与网络实战
  Linux socket/epoll、DPDK 官方文档与 demo、抓包与协议
        ↓
阶段 C  C++ 按需（策略层 / 框架）
  04 Modern C++ → 08 并发 → 09/10 C++17/20 → 13 性能工程（可选）
```

### HFT / 低延迟推荐顺序

| 优先级 | 目录 | 说明 |
|--------|------|------|
| **必学** | [00-Linux-Kernel-DPDK-Network-C](./00-Linux-Kernel-DPDK-Network-C/) | C 数据面核心 |
| 高 | [08-Cpp-Concurrency](./08-Cpp-Concurrency/) | 多线程、锁、内存序 |
| 中 | [04-Effective-Modern-C++](./04-Effective-Modern-C++/) | 现代 C++，写策略/业务时补 |
| 按需 | `13-Modern-C++-Performance-Engineering` | 低延迟、无锁、CPU 亲和（见下方可选拓展） |

> 通用 C++ 主线（01–10）仍保留；不走 HFT 时可按下方 **C++ 学习顺序** 推进。

## C++ 主线（01–10）

| 顺序 | 目录 | 书籍 | 侧重 |
|------|------|------|------|
| 1 | [01-C++Primer](./01-C++Primer/) | 《C++ Primer 第5版》 | 语法、标准库基础 |
| 2 | [02-Effective-C++](./02-Effective-C++/) | 《Effective C++ 第三版》 | 基础编码规范 |
| 3 | [03-More-Effective-C++](./03-More-Effective-C++/) | 《More Effective C++》 | 进阶语法、设计技巧 |
| 4 | [04-Effective-Modern-C++](./04-Effective-Modern-C++/) | 《Effective Modern C++》 | 移动语义、lambda、类型推导等现代特性 |
| 5 | [05-Effective-STL](./05-Effective-STL/) | 《Effective STL》 | STL 最佳实践 |
| 6 | [06-STL-Source-Analysis](./06-STL-Source-Analysis/) | 《STL源码剖析》 | STL 底层原理 |
| 7 | [07-Cpp-Object-Model](./07-Cpp-Object-Model/) | 《深度探索C++对象模型》 | 对象内存布局、多态底层 |
| 8 | [08-Cpp-Concurrency](./08-Cpp-Concurrency/) | 《C++并发编程实战》 | 线程、同步、内存模型 |
| 9 | [09-C++17-The-Complete-Guide](./09-C++17-The-Complete-Guide/) | 《C++17 - The Complete Guide》（Josuttis） | 结构化绑定、折叠表达式、并行 STL、`string_view` 等 C++17 过渡特性 |
| 10 | [10-C++20-The-Complete-Guide](./10-C++20-The-Complete-Guide/) | 《C++20 - The Complete Guide》（Josuttis） | Concepts、Modules、Coroutines、Ranges 等 C++20 标准 |

## C 语言学习顺序

见 **[00-Linux-Kernel-DPDK-Network-C](./00-Linux-Kernel-DPDK-Network-C/)**：阶段 1（01–04）→ 阶段 2（05 GNU-C）→ 内核 / DPDK / 网络源码。

## C++ 学习顺序

1. 先吃透 **04-Effective-Modern-C++**（C++11/14/17），建立现代 C++ 基础认知
2. 完成 STL、对象模型、并发等底层章节（05–08）
3. 读 **09-C++17-The-Complete-Guide**：结构化绑定、折叠表达式、并行 STL、`string_view` 等——HFT 技术栈里 17→20 的过渡基线
4. 最后切入 **10-C++20-The-Complete-Guide**，理解 C++20 是对 C++17 的升级拓展（Concepts、Ranges、Coroutines 等）

## 学习提示

- **HFT / 数据面**：优先 **00** + `00/01–05`；C++ 在能写策略后再补
- **C++ 经典与现代**：Effective 系列 + Modern C++ 衔接；**原理 > 语法版本**
- **C 的定位**：Linux 内核、DPDK、网络数据面；HFT 建议 **C 在前**
- **对象模型**：`07-Cpp-Object-Model/` 与 `00-.../02-Pointers-on-C/` 对照理解内存布局
- **全程保持**：笔记与代码写在对应书籍目录下，方便按书复盘

## 可选拓展（量化 / 低延迟方向）

| 目录（按需追加） | 书籍 | 侧重 |
|------------------|------|------|
| `12-Practical-C++20-Financial-Programming` | 《Practical C++20 Financial Programming》 | 金融量化 C++20 实战 |
| `13-Modern-C++-Performance-Engineering` | 《Modern C++ Performance Engineering》 | C++17/20 低延迟、无锁、CPU 优化 |
