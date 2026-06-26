# rust-quant-hft-handbook

本仓库收录 **Rust 全栈量化** + **HFT 微秒级低延迟** 学习笔记，配套原理拆解、可运行源码与工程实践。

**技术板块已封顶（`00`–`16`）** — **文件夹编号 = 推荐阅读顺序**（见下与 [LEARNING-CHAIN.md](./LEARNING-CHAIN.md)）。

→ 一眼进阶路径：[LEARNING-CHAIN.md](./LEARNING-CHAIN.md)  
→ 板块对照：[CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

---

## 🗺️ 推荐阅读顺序（= 文件夹编号）

```
00 业务 → 01 CSAPP → 02 Hennessy → 03 SysPerf → 04 BPF
→ 05 LKD → 06 ULK → 07 Gorman → 08 TLPI
→ 09 自制 OS/CPU → 10 陈硕 PNP/muduo → 11 UNP
→ 12 TCP/IP → 13 Rosen → 14 DPDK
→ 15 HFT 工程 → 16 Rust 量化
```

| 文件夹 | 模块 |
|:------:|------|
| **00** | [Trading and Exchanges](./00-Trading-and-Exchanges/) — Harris · LOB（练手：[00-practice-go-dex](./00-Trading-and-Exchanges/00-practice-go-dex/)） |
| **01** | [CSAPP-3rd](./01-CSAPP-3rd/) — 知其所以然 · 程序与硬件 |
| **02** | [Computer-Architecture-6th](./02-Computer-Architecture-6th/) — Hennessy · 体系结构（紧接 01） |
| **03** | [Systems-Performance-2nd](./03-Systems-Performance-2nd/) — 知其然 · 性能方法论 |
| **04** | [BPF-Performance-Tools](./04-BPF-Performance-Tools/) — eBPF / bpftrace（紧接 03） |
| **05** | [Linux-Kernel-Development](./05-Linux-Kernel-Development/) — LKD |
| **06** | [Understanding-Linux-Kernel](./06-Understanding-Linux-Kernel/) — ULK（紧接 05） |
| **07** | [Linux-Virtual-Memory-Manager](./07-Linux-Virtual-Memory-Manager/) — Gorman |
| **08** | [The-Linux-Programming-Interface](./08-The-Linux-Programming-Interface/) — TLPI |
| **09** | [system-low-level-hands-on](./09-system-low-level-hands-on/) — 30 天 OS / MikanOS / CPU |
| **10** | [Practical-Network-Programming](./10-Practical-Network-Programming/) — PNP / muduo |
| **11** | [UNP-Vol1](./11-UNP-Vol1/) |
| **12** | [TCP-IP-Illustrated-Vol1](./12-TCP-IP-Illustrated-Vol1/) |
| **13** | [Linux-Kernel-Networking](./13-Linux-Kernel-Networking/) — Rosen |
| **14** | [DPDK-Low-Latency-Network](./14-DPDK-Low-Latency-Network/) |
| **15** | [HFT-Low-Latency-Practice](./15-HFT-Low-Latency-Practice/) |
| **16** | [Rust-Quant-Trading-Guide](./16-Rust-Quant-Trading-Guide/) |

> **内核段：** `05` LKD → `06` ULK → `07` Gorman → `08` TLPI → `09` 自制 OS → 网络栈 `10`–`14` → 工程 `15`–`16`。

小节级读/跳 → [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) · 书目裁剪 → [READING-LIST.md](./READING-LIST.md)

| 标签 | 含义 |
|------|------|
| 🔴 **必读** | HFT 热路径 |
| 🟡 **选读** | 后补或场景触发 |
| ⚪ **跳过** | 默认不读 |

---

## 🛠️ 技术栈

- 主力语言：Rust
- 核心库：RustQuant、Barter-rs、Tokio、io_uring
- 学习辅助：NotebookLM + Cursor

## 📌 维护规范

- 顶层 **`00-` ~ `16-`** 封顶，不再扩展
- 笔记 / 源码 / 配图分区（`code/`、`assets/`）
- 外部书目只建索引，不 duplicate 全文笔记
