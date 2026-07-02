# C++ 学习链 · 外部仓库索引

**定位：** [cshonor/cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) 的 **HFT 主仓库入口** — 笔记与代码 **留在外部仓库**，本目录（[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study) 内）只负责 **排序、里程碑、与 00–18 的衔接**。

> **你为什么需要它：** 本仓库 **`10` PNP / muduo**、**`15` HFT 工程** 都是 **C++**；**`16` Rust** 之前若 C++ 零基础，读 muduo / 低延迟引擎会像看天书。

---

## 笔记仓库（外部 · 在这里写）

| 入口 | 链接 |
|------|------|
| **仓库首页** | [github.com/cshonor/cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) |
| **C 语言（02 对应）** | [11-Linux-Kernel-DPDK-Network-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C) |
| 全书目录 | [README.md（外部）](https://github.com/cshonor/cpp-learning-notes/blob/main/README.md) |
| 本仓库 C++ 编排 | [OUTLINE.md](./OUTLINE.md) |

**克隆（C + C++ 笔记共用一个外部仓）：**

```bash
git clone https://github.com/cshonor/cpp-learning-notes.git
```

→ **C 语言读序与裁剪** 见本仓 [02-c-programming](../02-c-programming/)（HFT 槽位 `02`；外部目录 `11-…`）

---

## 在主学习链里插在哪？

```
… → 08 TLPI → 07 自制 OS（可选）
         ↓
    【17 C++ · 本索引】  ← 开 10 PNP 之前至少读完 Modern C++
         ↓
    10 PNP → 11 UNP → … → 16 HFT（C++ 引擎）→ 17 Rust
```

| 阶段 | 本仓库模块 | C++ 外部仓要读到哪 |
|------|------------|-------------------|
| 打底 | **01 CSAPP**（C + 内存 + 并发直觉） | 可选：`01-C++Primer` Part I（与 CSAPP 并行） |
| 系统 | **05–09** | 不必硬啃 C++ |
| **开写 C++ 网络前** | → **08** → **09 PNP** | 🔴 **`04-Effective-Modern-C++`** 必过 |
| **开 HFT 引擎前** | → **16 HFT** | 🔴 **`08-Cpp-Concurrency`** + 🟡 **`07-Cpp-Object-Model`** |
| 进阶 | 15 之后 / 与 17 Rust 对照 | `09-C++20-The-Complete-Guide` |

完整里程碑 → [OUTLINE.md](./OUTLINE.md)

---

## 和「还没怎么学 C++」的对照

| 你的状态 | 建议 |
|----------|------|
| CSAPP 还没过完 | **先 01**，C++ 只开 Primer 语法扫盲，别深挖 |
| CSAPP + TLPI 已有体感 | **集中 2–4 周刷 `04-Effective-Modern-C++`**，再开 **10 PNP** |
| 想直接碰 muduo / HFT | **停** — 先 Modern C++ + 并发，否则调 `std::move`/线程会卡死 |

**一句话：** C++ 不是 Day 0 语言；**在会 C + 会 Linux 用户态（TLPI）之后、写 muduo 之前** 上 C++ 最省时间。

---

## 交叉阅读

| 本仓库 | 外部 C++ 仓 |
|--------|-------------|
| [01 CSAPP](../01-CSAPP-3rd/) Ch12 并发 | → [08-Cpp-Concurrency](https://github.com/cshonor/cpp-learning-notes/tree/main/08-Cpp-Concurrency) |
| [01 CSAPP](../01-CSAPP-3rd/) Ch6 缓存 | → [07-Cpp-Object-Model](https://github.com/cshonor/cpp-learning-notes/tree/main/07-Cpp-Object-Model) |
| [10 PNP / muduo](../10-Practical-Network-Programming/) | 前置 [04-Effective-Modern-C++](https://github.com/cshonor/cpp-learning-notes/tree/main/04-Effective-Modern-C++) |
| [16 HFT](../17-HFT-Low-Latency-Practice/) | 前置 Modern + Concurrency + Object Model |

← [总路线 ../LEARNING-CHAIN.md](../LEARNING-CHAIN.md) · [READING-LIST § C++](../READING-LIST.md#补充-c-学习链17-文件夹)
