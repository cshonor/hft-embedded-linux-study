# C++ 学习链 · 里程碑与 HFT 插入顺序

> **笔记正文：** 本目录 M0–M5 六个模块（M0、M1、M3–M5 自 [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) 复制；M2 = C++ 网络编程 muduo/PNP）  
> **HFT 读序 = 模块编号 M0→M5** — 实际顺序 M0→M1→M2→M3（穿插）→M4，M5 可选，以本表里程碑为准。

---

## 总原则

| 原则 | 说明 |
|------|------|
| **C 先于 C++** | [02 CSAPP](../02-computer-systems/) + [01 C](../01-c-language/) — C++ 是「带 RAII 的 C++」 |
| **Modern 先于 muduo** | [M2 PNP](./M2-cpp-network-programming/) 是 C++ 工程；**`01-Effective-Modern-C++` 是硬门槛** |
| **并发先于 HFT 引擎** | [14 HFT](../14-hft-engineering/)；**`02-Cpp-Concurrency` 必过** |
| **原理 > 语法版本** | Effective + Modern C++11/14 打底，再 17/20 |

---

## 里程碑（按 HFT 链插入）

### M0 · 语法扫盲（可选 · 与 01 CSAPP 并行）

| 目录 | 书目 | 何时 |
|------|------|------|
| [0-C++Primer](./M0-entry-syntax/01-C++Primer/) | C++ Primer 5e | **02 CSAPP** Ch3–5 后；只刷 **Part I + 标准库基础** |

**验收：** 能写 `vector`/`string`、引用、类、析构；不在此阶段啃模板元编程。

---

### M1 · 开 PNP 前必达 🔴

| 目录 | 书目 | 何时 |
|------|------|------|
| [1-Effective-Modern-C++](./M1-modern-cpp/01-Effective-Modern-C++/) | Effective Modern C++ | **03 TLPI 之后、M2 PNP 之前** |

**必会：** RAII、智能指针、`move`/完美转发、lambda、`=delete`/`=default`、`constexpr` 直觉。

**验收：** 能读 muduo 里 `shared_ptr` / 回调 / 移动语义不懵 → 再开 [M2 PNP](./M2-cpp-network-programming/)。

---

### M4 · 开 HFT 引擎前 🔴

| 目录 | 书目 | 何时 |
|------|------|------|
| [2-Cpp-Concurrency](./M4-deep-principles/02-Cpp-Concurrency/) | C++ 并发编程实战 | **03.5–12 网络栈进行中或之后、14 HFT 之前** |
| [2-Cpp-Object-Model](./M4-deep-principles/01-Cpp-Object-Model/) | 深度探索 C++ 对象模型 | 与 Concurrency **并行或略前** |

**验收：** 能写 mutex/condition_variable、理解 data race；能解释类大小、对齐、继承布局。

---

### M3 · STL 与规范（PNP 期间穿插）🟡

| 目录 | 书目 | 何时 |
|------|------|------|
| [3-Effective-C++](./M3-engineering-standards/01-Effective-C++/) | Effective C++ | M1 之后按需 |
| [3-More-Effective-C++](./M3-engineering-standards/02-More-Effective-C++/) | More Effective C++ | 同上 |
| [3-Effective-STL](./M3-engineering-standards/03-Effective-STL/) | Effective STL | **M2 PNP** 写缓冲区 / 容器时 |
| [3-STL-Source-Analysis](./M3-engineering-standards/04-STL-Source-Analysis/) | STL 源码剖析 | 时间紧可后补 |

---

### M5 · C++17 / C++20（17 之后 / 与 Rust 对照）⚪

| 目录 | 书目 | 何时 |
|------|------|------|
| [4-C++17-The-Complete-Guide](./M5-advanced-standards/01-C++17-The-Complete-Guide/) | C++17 | HFT 主线进行中可穿插 |
| [4-C++20-The-Complete-Guide](./M5-advanced-standards/02-C++20-The-Complete-Guide/) | C++20 | **14 HFT 主线跑通后**；Concepts / Coroutines / Modules |

---

## 一张图 · 和本仓库

```
02 CSAPP → 01 C ──────────────────────┐
                                      │ M0 可选 Primer
03 TLPI ──→ 03.5 UNP（可选并行）    │
                ↓                     │
           【04 · M1 Modern C++】◄────┘
                ↓
           M2 PNP → 11 TCP/IP → 12 内核网络 → 13 DPDK → 14 HFT
                ↓
           【04 · M4 并发 + 对象模型】
                ↓
           14 HFT（C++ 引擎）
                ↓
           18 Rust + 【04 · M5 C++17/20 可选】
```

---

## 最短路径（时间紧）

1. **`01-Effective-Modern-C++`**（全书）
2. **`02-Cpp-Concurrency`**（线程 + 同步 + 内存模型章）
3. **`01-Cpp-Object-Model`**（选章：对象布局、继承、虚函数）

其余 Effective / STL / C++17/20 **边做 14 HFT 边补**。

---

← [04 导读](./README.md) · [README](../README.md) · [01 C](../01-c-language/)
