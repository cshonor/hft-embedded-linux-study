# C++ 学习链 · 里程碑与 HFT 插入顺序

> **笔记正文：** 本目录 M0–M5 七个模块（M0、M1、M2、M2.5、M4、M5 自 [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) 复制；M3 = C++ 网络编程 muduo/PNP）  
> **HFT 读序 = 模块编号** — 实际顺序 M0→M1→M2 对象模型→M2.5 并发→M3 网络编程→M4（穿插），M5 可选，以本表里程碑为准。

---

## 总原则

| 原则 | 说明 |
|------|------|
| **C 先于 C++** | [02 CSAPP](../02-computer-systems/) + [01 C](../01-c-language/) — C++ 是「带 RAII 的 C++」 |
| **Modern 先于 muduo** | [M3 PNP](./M3-cpp-network-programming/) 是 C++ 工程；**`01-Effective-Modern-C++` 是硬门槛** |
| **原理先于网络、并发先于 muduo** | M2 对象模型 → M2.5 并发 → 再开 M3 PNP；[14 HFT](../14-hft-engineering/) 前 `01-Cpp-Concurrency` 必过 |
| **原理 > 语法版本** | Effective + Modern C++11/14 打底，再 17/20 |

---

## 里程碑（按 HFT 链插入）

### M0 · 语法扫盲（可选 · 与 01 CSAPP 并行）

| 目录 | 书目 | 何时 |
|------|------|------|
| [0-C++Primer](./M0-entry-syntax/01-C++Primer/) | C++ Primer 5e | **02 CSAPP** Ch3–5 后；只刷 **Part I + 标准库基础** |

**验收：** 能写 `vector`/`string`、引用、类、析构；不在此阶段啃模板元编程。

---

### M1 · 现代 C++ 门槛 🔴

| 目录 | 书目 | 何时 |
|------|------|------|
| [1-Effective-Modern-C++](./M1-modern-cpp/01-Effective-Modern-C++/) | Effective Modern C++ | **03 TLPI 之后、M2 对象模型之前** |

**必会：** RAII、智能指针、`move`/完美转发、lambda、`=delete`/`=default`、`constexpr` 直觉。

**验收：** 能读 muduo 里 `shared_ptr` / 回调 / 移动语义不懵 → 再开 M2 对象模型。

---

### M2 · 对象模型 🔴

| 目录 | 书目 | 何时 |
|------|------|------|
| [M2-Cpp-Object-Model](./M2-object-model/01-Cpp-Object-Model/) | 深度探索 C++ 对象模型 | **M1 之后、M2.5 并发之前** |

**验收：** 能解释类大小、对齐、继承布局、虚函数代价、placement new。

---

### M2.5 · 并发编程 🔴

| 目录 | 书目 | 何时 |
|------|------|------|
| [2.5-Cpp-Concurrency](./M2.5-concurrency/01-Cpp-Concurrency/) | C++ 并发编程实战 | **M2 对象模型之后、M3 PNP 之前**（muduo 是 one loop per thread，先懂线程再读） |

**验收：** 能写 mutex/condition_variable、理解 data race、能解释 6 种内存序。

---

### M3 · C++ 网络编程（muduo / PNP）🔴

| 目录 | 书目 | 何时 |
|------|------|------|
| [M3-cpp-network-programming](./M3-cpp-network-programming/) | 陈硕《Linux 多线程服务端编程》 | **M2.5 并发之后**；C 侧对照 03.5 UNP |

**验收：** 能手写 epoll Reactor、讲清 muduo Buffer/TcpConnection 生命周期、异步日志。

---

### M4.0 / M4.5 · STL 与规范（M3 PNP 期间穿插）🟡

| 目录 | 书目 | 何时 |
|------|------|------|
| [4.0-Effective-STL](./M4.0-STL/01-Effective-STL/) | Effective STL | **M3 PNP** 写缓冲区 / 容器时（亦可与 M1 EMC++ 配对读） |
| [4.0-STL-Source-Analysis](./M4.0-STL/02-STL-Source-Analysis/) | STL 源码剖析 | 时间紧可后补 |
| [4.5-Effective-C++](./M4.5-Classic-Effective/01-Effective-C++/) | Effective C++ | M1 之后按需 |
| [4.5-More-Effective-C++](./M4.5-Classic-Effective/02-More-Effective-C++/) | More Effective C++ | 同上 |

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
           【04 · M2 对象模型】
                ↓
           【04 · M2.5 并发编程】
                ↓
           M3 PNP/muduo → 11 TCP/IP → 12 内核网络 → 13 DPDK
                ↓
           14 HFT（C++ 引擎）
                ↓
           18 Rust + 【04 · M5 C++17/20 可选】
```

---

## 最短路径（时间紧）

1. **`01-Effective-Modern-C++`**（全书）
2. **`01-Cpp-Object-Model`**（选章：对象布局、继承、虚函数）
3. **`01-Cpp-Concurrency`**（线程 + 同步 + 内存模型章）

其余 Effective / STL / C++17/20 **边做 14 HFT 边补**。

---

← [04 导读](./README.md) · [README](../README.md) · [01 C](../01-c-language/)
