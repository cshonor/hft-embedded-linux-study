# C++ 学习链 · 里程碑与 HFT 插入顺序

> **笔记正文在外部：** [cshonor/cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)  
> 外部仓按 **01–09 书目** 分目录；**HFT 读序 ≠ 外部仓文件夹编号** — 以本表为准。

---

## 总原则

| 原则 | 说明 |
|------|------|
| **C 先于 C++** | [01 CSAPP](../01-CSAPP-3rd/) 图景 + **[02 C](../02-c-programming/)**（本地 `01`–`05` 笔记）— C++ 是「带 RAII 的 C++」不是第二门语言 |
| **Modern 先于 muduo** | [10 PNP](../10-Practical-Network-Programming/) 是 C++ 工程；**`04-Effective-Modern-C++` 是硬门槛** |
| **并发先于 HFT 引擎** | [16 HFT](../17-HFT-Low-Latency-Practice/) 热路径 = 多线程 + 无锁；**`08-Cpp-Concurrency` 必过** |
| **原理 > 语法版本** | Effective 老书 + Modern C++11/14 打底，再 C++20 — 与 [外部仓 README](https://github.com/cshonor/cpp-learning-notes/blob/main/README.md) 一致 |

---

## 里程碑（按 HFT 链插入）

### M0 · 语法扫盲（可选 · 与 01 CSAPP 并行）

| 外部目录 | 书目 | 何时 |
|----------|------|------|
| [01-C++Primer](https://github.com/cshonor/cpp-learning-notes/tree/main/01-C++Primer) | C++ Primer 5e | **01 CSAPP** 读到 Ch3–5 后；只刷 **Part I + 标准库基础** |

**验收：** 能写 `vector`/`string`、引用、类、析构；不在此阶段啃模板元编程。

---

### M1 · 开 PNP 前必达 🔴

| 外部目录 | 书目 | 何时 |
|----------|------|------|
| [04-Effective-Modern-C++](https://github.com/cshonor/cpp-learning-notes/tree/main/04-Effective-Modern-C++) | Effective Modern C++ | **07 TLPI 之后、10 PNP 之前**（08 MikanOS 可并行） |

**必会：** RAII、智能指针、`move`/完美转发、lambda、`=delete`/`=default`、`constexpr` 直觉。

**验收：** 能读 muduo 里 `shared_ptr` / 回调 / 移动语义不懵 → 再开 [10 PNP](../10-Practical-Network-Programming/)。

---

### M2 · 开 HFT 引擎前 🔴

| 外部目录 | 书目 | 何时 |
|----------|------|------|
| [08-Cpp-Concurrency](https://github.com/cshonor/cpp-learning-notes/tree/main/08-Cpp-Concurrency) | C++ 并发编程实战 | **10–14 网络栈进行中或之后、17 HFT 之前** |
| [07-Cpp-Object-Model](https://github.com/cshonor/cpp-learning-notes/tree/main/07-Cpp-Object-Model) | 深度探索 C++ 对象模型 | 与 Concurrency **并行或略前**（理解内存布局 / 虚表） |

**验收：** 能写 mutex/condition_variable、理解 data race；能解释类大小、对齐、继承布局（对接 CSAPP 伪共享 / HFT 缓存行）。

---

### M3 · STL 与规范（PNP 期间穿插）🟡

| 外部目录 | 书目 | 何时 |
|----------|------|------|
| [02-Effective-C++](https://github.com/cshonor/cpp-learning-notes/tree/main/02-Effective-C++) | Effective C++ | M1 之后按需 |
| [03-More-Effective-C++](https://github.com/cshonor/cpp-learning-notes/tree/main/03-More-Effective-C++) | More Effective C++ | 同上 |
| [05-Effective-STL](https://github.com/cshonor/cpp-learning-notes/tree/main/05-Effective-STL) | Effective STL | **10 PNP** 写缓冲区 / 容器时 |
| [06-STL-Source-Analysis](https://github.com/cshonor/cpp-learning-notes/tree/main/06-STL-Source-Analysis) | STL 源码剖析 | 时间紧可后补 |

---

### M4 · C++20（15 之后 / 与 17 Rust 对照）⚪

| 外部目录 | 书目 | 何时 |
|----------|------|------|
| [09-C++20-The-Complete-Guide](https://github.com/cshonor/cpp-learning-notes/tree/main/09-C++20-The-Complete-Guide) | Josuttis C++20 | **17 HFT 主线跑通后**；Concepts / Coroutines / Modules |

外部仓可选：**Practical C++20 Financial Programming** · **Modern C++ Performance Engineering** — 见 [外部 README · 可选拓展](https://github.com/cshonor/cpp-learning-notes#可选拓展量化--低延迟方向)。

---

## 一张图 · 和本仓库 00–18

```
01 CSAPP → 02 C ──────────────────────┐
                                      │ M0 可选 Primer
07 TLPI ──→ 08 MikanOS（可选并行）    │
                ↓                     │
           【09 · M1 Modern C++】◄────┘
                ↓
           10 PNP / 11 UNP / 12–14
                ↓
           【09 · M2 并发 + 对象模型】
                ↓
           17 HFT（C++ 引擎）
                ↓
           18 Rust + 【09 · M4 C++20 可选】
```

---

## 最短路径（时间紧）

若 **只想尽快进 PNP + HFT**，外部仓最少：

1. **`04-Effective-Modern-C++`**（全书）
2. **`08-Cpp-Concurrency`**（线程 + 同步 + 内存模型章）
3. **`07-Cpp-Object-Model`**（选章：对象布局、继承、虚函数）

其余 Effective / STL 源码 / C++20 **边做 17 HFT 边补**。

---

← [09 导读](./README.md) · [LEARNING-CHAIN](../LEARNING-CHAIN.md) · [02 C](../02-c-programming/)
