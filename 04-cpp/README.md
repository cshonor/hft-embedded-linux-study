# 04 · C++ 学习链

**文件夹 `04`** · [OUTLINE](./OUTLINE.md) · [README](../README.md)

> **定位：** 本仓库 **M5 PNP / muduo**（C++ 网络编程）、**`17` HFT** 的 C++ 前置。  
> **笔记正文已在本目录：** 自 [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) 复制，按 M0–M5 六个模块组织（每个模块内部书从 01 开始编号）。  
> **C 语言** 在本仓 [01-c-language](../01-c-language/)（不要在这里重复啃 K&R）。

上游全书目录备份 → [README.external.md](./README.external.md)

---

## 书单（本目录）

| 模块 | 目录 | 书籍 |
|------|------|------|
| M0 入门语法 | [0-C++Primer](./M0-entry-syntax/01-C++Primer/) | C++ Primer 5e |
| M1 现代 C++ 🔴 | [1-Effective-Modern-C++](./M1-modern-cpp/01-Effective-Modern-C++/) | Effective Modern C++ |
| M2 深入原理 🔴 | [2-Cpp-Object-Model](./M2-deep-principles/01-Cpp-Object-Model/) | 深度探索 C++ 对象模型 |
|  | [2-Cpp-Concurrency](./M2-deep-principles/02-Cpp-Concurrency/) | C++ 并发编程实战 |
| M3 工程规范 🟡 | [3-Effective-C++](./M3-engineering-standards/01-Effective-C++/) | Effective C++ |
|  | [3-More-Effective-C++](./M3-engineering-standards/02-More-Effective-C++/) | More Effective C++ |
|  | [3-Effective-STL](./M3-engineering-standards/03-Effective-STL/) | Effective STL |
|  | [3-STL-Source-Analysis](./M3-engineering-standards/04-STL-Source-Analysis/) | STL 源码剖析 |
| M4 进阶标准 ⚪ | [4-C++17-The-Complete-Guide](./M4-advanced-standards/01-C++17-The-Complete-Guide/) | C++17 Complete Guide |
|  | [4-C++20-The-Complete-Guide](./M4-advanced-standards/02-C++20-The-Complete-Guide/) | C++20 Complete Guide |
| **M5 C++ 网络编程** 🔴 | [M5-cpp-network-programming](./M5-cpp-network-programming/) | 陈硕《Linux 多线程服务端编程》（muduo / PNP 实验笔记 ×9） |

---

## 在主学习链里插在哪？

```
… → 03 TLPI → 03.5 UNP socket（C）
         ↓
    【04 C++ · 本目录】  ← 开 M5 前至少读完 Modern C++
         ↓
    M5 PNP/muduo（本目录）→ … → 14 HFT（C++ 引擎）→ 18 Rust
```

| 阶段 | 本仓库模块 | 本目录要读到哪 |
|------|------------|----------------|
| 打底 | **02 CSAPP** | 可选：[0-C++Primer](./M0-entry-syntax/01-C++Primer/) Part I |
| **开写 C++ 网络前** | → **M5 PNP**（本目录） | 🔴 [1-Effective-Modern-C++](./M1-modern-cpp/01-Effective-Modern-C++/) |
| **开 HFT 引擎前** | → **14 HFT** | 🔴 [2-Cpp-Concurrency](./M2-deep-principles/02-Cpp-Concurrency/) + 🟡 [2-Cpp-Object-Model](./M2-deep-principles/01-Cpp-Object-Model/) |
| 进阶 | 17 之后 / 与 Rust 对照 | [4-C++17](./M4-advanced-standards/01-C++17-The-Complete-Guide/) · [4-C++20](./M4-advanced-standards/02-C++20-The-Complete-Guide/) |

完整里程碑 → [OUTLINE.md](./OUTLINE.md)

---

## 和「还没怎么学 C++」的对照

| 你的状态 | 建议 |
|----------|------|
| CSAPP 还没过完 | **先 02**，C++ 只开 Primer 语法扫盲 |
| CSAPP + TLPI 已有体感 | **集中刷 `01-Effective-Modern-C++`**，再开 **M5 PNP** |
| 想直接碰 muduo / HFT | **停** — 先 Modern C++ + 并发 |

**一句话：** C++ 在会 C + 会 Linux 用户态（TLPI）之后、写 muduo 之前上最省时间。

---

## 交叉阅读

| 本仓库 | 本目录 |
|--------|--------|
| [02 CSAPP](../02-computer-systems/) Ch12 并发 | → [2-Cpp-Concurrency](./M2-deep-principles/02-Cpp-Concurrency/) |
| [02 CSAPP](../02-computer-systems/) Ch6 缓存 | → [2-Cpp-Object-Model](./M2-deep-principles/01-Cpp-Object-Model/) |
| [M5 PNP / muduo](./M5-cpp-network-programming/) | 前置 [1-Effective-Modern-C++](./M1-modern-cpp/01-Effective-Modern-C++/)；C 侧对照 [03.5 UNP](../03.5-unix-network-api/) |
| [14 HFT](../14-hft-engineering/) | 前置 Modern + Concurrency + Object Model |
| [01 C](../01-c-language/) | C 数据面；与 C++ **分工不重复** |

← [README](../README.md) · [READING-LIST](../READING-LIST.md)
