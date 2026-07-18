# 09 · C++ 学习链

**文件夹 `09`** · [OUTLINE](./OUTLINE.md) · [LEARNING-CHAIN](../LEARNING-CHAIN.md)

> **定位：** 本仓库 **`10` PNP / muduo**、**`17` HFT** 的 C++ 前置。  
> **笔记正文已在本目录：** 自 [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) 复制的 **01–10**（C++ 主线）。  
> **C 语言** 在本仓 [02-c-programming](../02-c-programming/)（不要在这里重复啃 K&R）。

上游全书目录备份 → [README.external.md](./README.external.md)

---

## 书单（本目录）

| 顺序 | 目录 | 书籍 |
|------|------|------|
| 1 | [01-C++Primer](./01-C++Primer/) | C++ Primer 5e |
| 2 | [02-Effective-C++](./02-Effective-C++/) | Effective C++ |
| 3 | [03-More-Effective-C++](./03-More-Effective-C++/) | More Effective C++ |
| 4 | [04-Effective-Modern-C++](./04-Effective-Modern-C++/) | Effective Modern C++ |
| 5 | [05-Effective-STL](./05-Effective-STL/) | Effective STL |
| 6 | [06-STL-Source-Analysis](./06-STL-Source-Analysis/) | STL 源码剖析 |
| 7 | [07-Cpp-Object-Model](./07-Cpp-Object-Model/) | 深度探索 C++ 对象模型 |
| 8 | [08-Cpp-Concurrency](./08-Cpp-Concurrency/) | C++ 并发编程实战 |
| 9 | [09-C++17-The-Complete-Guide](./09-C++17-The-Complete-Guide/) | C++17 Complete Guide |
| 10 | [10-C++20-The-Complete-Guide](./10-C++20-The-Complete-Guide/) | C++20 Complete Guide |

---

## 在主学习链里插在哪？

```
… → 07 TLPI → 08 MikanOS（可选）
         ↓
    【09 C++ · 本目录】  ← 开 10 PNP 前至少读完 Modern C++
         ↓
    10 PNP → … → 17 HFT（C++ 引擎）→ 18 Rust
```

| 阶段 | 本仓库模块 | 本目录要读到哪 |
|------|------------|----------------|
| 打底 | **01 CSAPP** | 可选：[01-C++Primer](./01-C++Primer/) Part I |
| **开写 C++ 网络前** | → **10 PNP** | 🔴 [04-Effective-Modern-C++](./04-Effective-Modern-C++/) |
| **开 HFT 引擎前** | → **17 HFT** | 🔴 [08-Cpp-Concurrency](./08-Cpp-Concurrency/) + 🟡 [07-Cpp-Object-Model](./07-Cpp-Object-Model/) |
| 进阶 | 17 之后 / 与 Rust 对照 | [09-C++17](./09-C++17-The-Complete-Guide/) · [10-C++20](./10-C++20-The-Complete-Guide/) |

完整里程碑 → [OUTLINE.md](./OUTLINE.md)

---

## 和「还没怎么学 C++」的对照

| 你的状态 | 建议 |
|----------|------|
| CSAPP 还没过完 | **先 01**，C++ 只开 Primer 语法扫盲 |
| CSAPP + TLPI 已有体感 | **集中刷 `04-Effective-Modern-C++`**，再开 **10 PNP** |
| 想直接碰 muduo / HFT | **停** — 先 Modern C++ + 并发 |

**一句话：** C++ 在会 C + 会 Linux 用户态（TLPI）之后、写 muduo 之前上最省时间。

---

## 交叉阅读

| 本仓库 | 本目录 |
|--------|--------|
| [01 CSAPP](../01-CSAPP-3rd/) Ch12 并发 | → [08-Cpp-Concurrency](./08-Cpp-Concurrency/) |
| [01 CSAPP](../01-CSAPP-3rd/) Ch6 缓存 | → [07-Cpp-Object-Model](./07-Cpp-Object-Model/) |
| [10 PNP / muduo](../10-Practical-Network-Programming/) | 前置 [04-Effective-Modern-C++](./04-Effective-Modern-C++/) |
| [17 HFT](../17-HFT-Low-Latency-Practice/) | 前置 Modern + Concurrency + Object Model |
| [02 C](../02-c-programming/) | C 数据面；与 C++ **分工不重复** |

← [LEARNING-CHAIN](../LEARNING-CHAIN.md) · [READING-LIST](../READING-LIST.md)
