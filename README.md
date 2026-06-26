# rust-quant-hft-handbook

本仓库收录 **Rust 全栈量化** + **HFT 微秒级低延迟** 学习笔记，配套原理拆解、可运行源码与工程实践。

**技术板块已封顶（`00`–`16`）** — 文件夹名排序；**推荐阅读顺序**见下与 [LEARNING-CHAIN.md](./LEARNING-CHAIN.md)。

→ 一眼进阶路径：[LEARNING-CHAIN.md](./LEARNING-CHAIN.md)  
→ 板块对照：[CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

---

## 🗺️ 推荐阅读顺序

```
00 业务 → 01 CSAPP → 02 SysPerf → 03 BPF
→ 04 Hennessy → 05 LKD → 08 ULK → 06 Gorman → 07 TLPI
→ 09 自制 OS/CPU → 10 陈硕 PNP/muduo → 11 UNP
→ 12 TCP/IP → 13 Rosen → 14 DPDK
→ 15 HFT 工程 → 16 Rust 量化
```

| 序号 | 文件夹 | 内容 |
|------|--------|------|
| 00 | [00-Trading-and-Exchanges](./00-Trading-and-Exchanges/) | Harris · LOB / 市场结构（练手：[00-practice-go-dex](./00-Trading-and-Exchanges/00-practice-go-dex/)） |
| 01 | [01-CSAPP-3rd](./01-CSAPP-3rd/) | **知其所以然** · 程序与硬件 |
| 02 | [02-Systems-Performance-2nd](./02-Systems-Performance-2nd/) | **知其然** · 性能方法论 |
| 03 | [03-BPF-Performance-Tools](./03-BPF-Performance-Tools/) | **工具落地** · eBPF / bpftrace |
| 04 | [04-Computer-Architecture-6th](./04-Computer-Architecture-6th/) | Hennessy · 体系结构（配 01） |
| 05 | [05-Linux-Kernel-Development](./05-Linux-Kernel-Development/) | LKD · 课+书 [LEARNING-PATH](./05-Linux-Kernel-Development/LEARNING-PATH.md) |
| 06 | [06-Linux-Virtual-Memory-Manager](./06-Linux-Virtual-Memory-Manager/) | Gorman · 虚拟内存 |
| 07 | [07-The-Linux-Programming-Interface](./07-The-Linux-Programming-Interface/) | **TLPI** · Linux 用户态系统编程 · epoll / mmap / 线程 |
| 08 | [08-Understanding-Linux-Kernel](./08-Understanding-Linux-Kernel/) | **ULK** · 内核实现细节（LKD↔Gorman 桥梁） |
| 09 | [09-system-low-level-hands-on](./09-system-low-level-hands-on/) | **系统底层动手** · 30 天 OS / MikanOS / CPU |
| 10 | [10-Practical-Network-Programming](./10-Practical-Network-Programming/) | **C++ 网络实战** · 陈硕 PNP / muduo |
| 11 | [11-UNP-Vol1](./11-UNP-Vol1/) | UNP · Socket API |
| 12 | [12-TCP-IP-Illustrated-Vol1](./12-TCP-IP-Illustrated-Vol1/) | TCP/IP 卷一 · 协议语义 |
| 13 | [13-Linux-Kernel-Networking](./13-Linux-Kernel-Networking/) | Rosen · 内核网络栈 |
| 14 | [14-DPDK-Low-Latency-Network](./14-DPDK-Low-Latency-Network/) | DPDK · 用户态旁路 |
| 15 | [15-HFT-Low-Latency-Practice](./15-HFT-Low-Latency-Practice/) | **动手实现** · C++ 低延迟工程 |
| 16 | [16-Rust-Quant-Trading-Guide](./16-Rust-Quant-Trading-Guide/) | **动手实现** · Rust 量化 |

> **系统段：** `07` TLPI → `08` ULK → `09` 自制 OS → `10` PNP 实战 → `11` UNP API。

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
