# hft-embedded-linux-study

> **GitHub：** [github.com/cshonor/hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)  
> 为 **HFT 低延迟 Linux 底层** 与 **嵌入式 Linux 无人机飞控** 双线学习打造的笔记与路线仓库。

本仓库收录 **Rust 全栈量化** + **HFT 微秒级低延迟** 学习笔记骨架，配套原理拆解、可运行源码索引与工程实践路线。

**技术板块 `00`–`18` 主线 + 嵌入式支线 `19`–`24`** — **文件夹编号 = 推荐阅读顺序**（见 [LEARNING-CHAIN.md](./LEARNING-CHAIN.md)）。

→ 一眼进阶路径：[LEARNING-CHAIN.md](./LEARNING-CHAIN.md)  
→ 板块对照：[CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)  
→ 完整路线图：[HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)

---

## 相关仓库（笔记写在哪）

| 仓库 | 用途 | 本仓对应 |
|------|------|----------|
| **[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)** | **本仓库** — 读序、OUTLINE、章节 scaffold | `00`–`24` 文件夹 |
| **[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)** | **C / C++ 详细笔记与代码** | [02 C](./02-c-programming/) → [`11-Linux-Kernel-DPDK-Network-C`](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C) · [09 C++](./09-cpp-learning-notes/) → `01`–`10` |
| [Computer-Networking](https://github.com/cshonor/Computer-Networking) | PNP / UNP 实战代码 | [10 PNP](./10-Practical-Network-Programming/) · [11 UNP](./11-UNP-Vol1/) |

**克隆本仓库：**

```bash
git clone https://github.com/cshonor/hft-embedded-linux-study.git
```

**C / C++ 笔记（与上表共用外部仓）：**

```bash
git clone https://github.com/cshonor/cpp-learning-notes.git
```


## 🗺️ HFT 主线阅读顺序（= 文件夹编号）

```
00 业务 → 01 CSAPP → 02 C 语言 → 03 Hennessy
→ 04 LKD → 05 ULK → 06 Gorman → 07 TLPI
→ 08 MikanOS/30天OS → 09 C++ → 10 陈硕 PNP/muduo → 11 UNP
→ 12 TCP/IP → 13 Rosen → 14 DPDK
→ 15 SysPerf → 16 BPF
→ 17 HFT 工程 → 18 Rust 量化
```

**可选支线 · 嵌入式 Linux：** `19 → … → 24`（建议 04–07 后再开）→ [HFT-READING-ROADMAP §六](./HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

| 文件夹 | 模块 |
|:------:|------|
| **00** | [Trading and Exchanges](./00-Trading-and-Exchanges/) |
| **01** | [CSAPP-3rd](./01-CSAPP-3rd/) — 程序与硬件图景 |
| **02** | [c-programming](./02-c-programming/) — **K&R · Pointers on C · GNU-C**（笔记 → [外部 11-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C)） |
| **03** | [Computer-Architecture-6th](./03-Computer-Architecture-6th/) — Hennessy |
| **04** | [Linux-Kernel-Development](./04-Linux-Kernel-Development/) — LKD |
| **05** | [Understanding-Linux-Kernel](./05-Understanding-Linux-Kernel/) — ULK |
| **06** | [Linux-Virtual-Memory-Manager](./06-Linux-Virtual-Memory-Manager/) — Gorman |
| **07** | [The-Linux-Programming-Interface](./07-The-Linux-Programming-Interface/) — TLPI |
| **08** | [system-low-level-hands-on](./08-system-low-level-hands-on/) — **01 MikanOS** / 02 30天 |
| **09** | [cpp-learning-notes](./09-cpp-learning-notes/) — C++ |
| **10** | [Practical-Network-Programming](./10-Practical-Network-Programming/) — PNP |
| **11** | [UNP-Vol1](./11-UNP-Vol1/) |
| **12** | [TCP-IP-Illustrated-Vol1](./12-TCP-IP-Illustrated-Vol1/) |
| **13** | [Linux-Kernel-Networking](./13-Linux-Kernel-Networking/) — Rosen |
| **14** | [DPDK-Low-Latency-Network](./14-DPDK-Low-Latency-Network/) |
| **15** | [Systems-Performance-2nd](./15-Systems-Performance-2nd/) |
| **16** | [BPF-Performance-Tools](./16-BPF-Performance-Tools/) |
| **17** | [HFT-Low-Latency-Practice](./17-HFT-Low-Latency-Practice/) |
| **18** | [Rust-Quant-Trading-Guide](./18-Rust-Quant-Trading-Guide/) |
| **19–24** | 嵌入式 Linux + 飞控 — 见 [路线图](./HFT-READING-ROADMAP.md) |

> **内核段：** `01` → **`02` C** → `03` Hennessy → `04`–`07` → `08` OS → `09` C++ → 网络 `10`–`14` → 性能 `15`–`16` → 工程 `17`–`18`。
