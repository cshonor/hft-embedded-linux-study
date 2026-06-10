# rust-quant-hft-handbook

本仓库收录 **Rust 全栈量化** + **HFT 微秒级低延迟** 学习笔记，配套原理拆解、可运行源码与工程实践。

**技术板块已封顶（`00`–`12`）** — 不再新增顶层编号文件夹；剩余工作为各目录内笔记、代码实验与跨模块联动。

→ 板块边界与对照逻辑：[CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

---

## 🗺️ HFT 阅读顺序（总纲）

> 详细小节级读/跳指引 → **[HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)**（建议先读此文件）

| 阶段 | 读什么 | 目的 |
|------|--------|------|
| **0** | [00-Trading-and-Exchanges](./00-Trading-and-Exchanges/) · LOB/市场结构 | 业务锚点，先懂撮合再写引擎 |
| **1** | [01-Systems-Performance-2nd](./01-Systems-Performance-2nd/) | 会量延迟、会排抖动 |
| **2** | [02-Linux-Kernel-Development-3rd](./02-Linux-Kernel-Development-3rd/) | 绑核、中断、调度 |
| **3** | [03-Linux-Virtual-Memory-Manager](./03-Linux-Virtual-Memory-Manager/) + [07-Computer-Architecture-6th](./07-Computer-Architecture-6th/) + [08-CSAPP-3rd](./08-CSAPP-3rd/) 内存章 | NUMA / TLB / 伪共享 |
| **4** | [04-TCP-IP](./04-TCP-IP-Illustrated-Vol1/) → [05-UNP](./05-UNP-Vol1/) → [08-CSAPP-3rd](./08-CSAPP-3rd/) 网络章 → [06-Rosen](./06-Linux-Kernel-Networking/) → **[12-DPDK](./12-DPDK-Low-Latency-Network/)** | 协议 → Socket → 内核栈 → **用户态旁路（网络闭环）** |
| **5** | [08-CSAPP-3rd](./08-CSAPP-3rd/) 优化/并发 + Hennessy 剩余 | 热路径代码优化 |
| **6** | Harris 剩余 + [09-BPF-Performance-Tools](./09-BPF-Performance-Tools/) | 生产观测 |
| **7** | [10-HFT-Low-Latency-Practice](./10-HFT-Low-Latency-Practice/) + [11-Rust-Quant-Trading-Guide](./11-Rust-Quant-Trading-Guide/) | **交易系统工程落地**（非网络技术） |

**推荐序号：** 0 → ① → ② → ③ → 外A → 外B → ④ → ⑤ → ⑥ → ⑦ → ⑧ → **⑫ DPDK** → 实战笔记

| 标签 | 含义 |
|------|------|
| 🔴 **必读** | HFT 热路径，不能 skip |
| 🟡 **选读** | 后补或场景触发（如订单走 TCP） |
| ⚪ **跳过** | 本仓库无笔记，默认不读 |

---

## 📚 仓库目录（`00`–`12` = 全部技术板块）

| 序号 | 文件夹 | 板块 | 内容 |
|------|--------|------|------|
| 00 | [00-Trading-and-Exchanges](./00-Trading-and-Exchanges/) | 交易理论 | Harris · 市场/LOB（29 章） |
| 01 | [01-Systems-Performance-2nd](./01-Systems-Performance-2nd/) | Linux 系统 | Gregg · 性能方法论 |
| 02 | [02-Linux-Kernel-Development-3rd](./02-Linux-Kernel-Development-3rd/) | Linux 系统 | Love · 内核调度/中断 |
| 03 | [03-Linux-Virtual-Memory-Manager](./03-Linux-Virtual-Memory-Manager/) | Linux 系统 | Gorman · 14 章 + 附录 [OUTLINE](./03-Linux-Virtual-Memory-Manager/OUTLINE.md) |
| 04 | [04-TCP-IP-Illustrated-Vol1](./04-TCP-IP-Illustrated-Vol1/) | **网络** | Stevens · 协议原理 → [外部仓库](https://github.com/cshonor/Computer-Networking) |
| 05 | [05-UNP-Vol1](./05-UNP-Vol1/) | **网络** | Stevens · Socket API → [外部仓库](https://github.com/cshonor/Computer-Networking) |
| 06 | [06-Linux-Kernel-Networking](./06-Linux-Kernel-Networking/) | **网络** | Rosen · 14 章 + 附录 [OUTLINE](./06-Linux-Kernel-Networking/OUTLINE.md) |
| 07 | [07-Computer-Architecture-6th](./07-Computer-Architecture-6th/) | 硬件 CPU | Hennessy · 缓存/MESI |
| 08 | [08-CSAPP-3rd](./08-CSAPP-3rd/) | 硬件 CPU | CSAPP · 12 章 + 附录 + Lab |
| 09 | [09-BPF-Performance-Tools](./09-BPF-Performance-Tools/) | 性能观测 | Gregg · eBPF |
| 10 | [10-HFT-Low-Latency-Practice](./10-HFT-Low-Latency-Practice/) | **工程落地** | 整机调优 / 对接 / 架构 / 压测（≠ 网络书） |
| 11 | [11-Rust-Quant-Trading-Guide](./11-Rust-Quant-Trading-Guide/) | Rust 开发 | 量化工程 · 无锁 |
| 12 | [12-DPDK-Low-Latency-Network](./12-DPDK-Low-Latency-Network/) | **网络** | DPDK 用户态旁路 · **网络栈闭环** |

每本书文件夹内 **README.md** = 必读/选读/跳过速查。

书目裁剪与映射表 → [READING-LIST.md](./READING-LIST.md)

---

## 🛠️ 技术栈

- 主力语言：Rust
- 核心库：RustQuant、Barter-rs、Tokio、io_uring
- 学习辅助：NotebookLM 书籍提炼 + Cursor 代码辅助

## 📌 仓库维护规范

- 根目录文件夹以 **`00-` ~ `12-` 序号前缀** 排序；**编号封顶，不再扩展顶层目录**
- 章节按序号独立拆分，单文件单一主题
- 笔记、源码、配图严格分区（`code/`、`assets/`）
- 外部书目只建索引 README，不 duplicate 另一仓库的全文笔记
- 跨模块对照统一维护在 [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)
