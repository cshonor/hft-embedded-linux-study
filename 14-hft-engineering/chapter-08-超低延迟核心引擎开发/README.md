# 第8章 超低延迟核心引擎开发（C++ 微秒征途）（索引）

> **原书第 8 章 · C++ – The Quest for Microsecond Latency**
> **内存模型 · 静态多态 · 内存池 · 模板 · 静态分析 · FX 实战**

← [chapter-07 无锁环](../chapter-07-无锁数据结构与内存布局/README.md) · [chapter-10 测量](../chapter-10-延迟测量与基准压测/README.md)

---

## 本章定位

原书 **Ch8** 在 OS / 网络 / 硬件之后，将视线聚焦于 HFT **最常用语言 C++**（C++11/14/17）。核心理念：

> **编译期能做完的，绝不留到运行时；迎合 CPU 缓存；剥离一切不必要的运行时开销。**

| 主题 | 本章 | 交叉 |
|------|------|------|
| 内存模型 / 原子序 | **8.1** | [Ch7 §5](../chapter-07-无锁数据结构与内存布局/7.5-CPP内存序.md) |
| 绑核 / Bypass / Hugepage | 8.6 实战 | [Ch5](../chapter-05-操作系统内核极致调优/README.md) · [Ch6](../chapter-06-低延迟网络与协议优化/README.md) |
| 无锁 IPC | 8.6 实战 | [Ch7 §4](../chapter-07-无锁数据结构与内存布局/7.4-共享内存IPC.md) |
| Gateway / Book / OMS | **8.7** | [Ch1 §1](../chapter-01-高频交易基础与生态/1.1-系统核心架构.md) |

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [8.1](./8.1-CPP内存模型.md) | C++ 内存模型 🔴 | 重排规则 + acquire/release — 多核正确性 |
| [8.2](./8.2-消除运行时决策.md) | 消除运行时决策 🔴 | CRTP 静态多态 · 禁 RTTI/虚函数 |
| [8.3](./8.3-动态内存与异常.md) | 动态内存分配与异常 | 池 + 栈 · 热点 no throw |
| [8.4](./8.4-模板与STL容器.md) | 模板与 STL 容器 | 编译期内联 · vector > list · 防 code bloat |
| [8.5](./8.5-静态分析.md) | 静态分析 | Klocwork/Cppcheck/Clang SA 抓并发 bug |
| [8.6](./8.6-FX实战架构清单.md) | FX 实战架构清单 🔴 | 绑核 + mmap 环 + Onload + Hugepage + 预热 |
| [8.7](./8.7-关键路径组件.md) | 关键路径组件 | Gateway/Book/Strategy/OMS 各自要点 |
| [8.8](./8.8-Java与Python边界.md) | Java / Python 边界 | 三语言分工总览 |

## 本章小结

| 原书 Ch8 主题 | 手段 |
|---------------|------|
| **内存模型** | acquire/release · relaxed · fence — **非默认 seq_cst** |
| **运行时决策** | CRTP 静态多态 · **禁 RTTI / 虚函数** |
| **内存 / 异常** | 池 + 栈 · **热点 no throw** |
| **模板** | 编译期内联 · **vector > list** · 防 code bloat |
| **静态分析** | Klocwork / Cppcheck / Clang SA |
| **FX 实战** | 多进程绑核 · mmap 环 · OpenOnload · Hugepage · 预热 |

**C++ 性能圣经落地后** → [chapter-09 Java/JVM](../chapter-09-java-jvm-低延迟系统/README.md) · [chapter-14 Python 混合](../chapter-14-python-高性能混合架构/README.md) · 策略：[chapter-13](../chapter-13-高频做市与套利策略/README.md)

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch8 §1 内存模型 | **本章 8.1** · Ch7 §5 |
| Ch8 §2 消除运行时决策 | **本章 8.2** |
| Ch8 §3 动态内存/异常 | **本章 8.3** · Ch7 §3 |
| Ch8 §4 模板 | **本章 8.4** |
| Ch8 §5 静态分析 | **本章 8.5** |
| Ch8 §6 FX 实战 | **本章 8.6–8.7** |
| Ch9 Java/JVM | **Ch9** |
| Ch10 Python | **Ch14** |

## C++ 热点速查（Do / Don't）

| Do | Don't |
|----|-------|
| **CRTP**、模板策略 | 虚函数多态 |
| **对象池 / 栈缓冲** | 热点 `malloc` / `vector` 扩容 |
| `memory_order_acquire/release` | 默认 seq_cst |
| `noexcept`、错误码 | 热点 `throw` |
| **Branch prediction 友好** | 深层 if-else · `dynamic_cast` |

→ [chapter-01 §5 语言选择](../chapter-01-高频交易基础与生态/1.5-编程语言选择.md)
