# rust-quant-hft-handbook

本仓库收录 **Rust 全栈量化** + **HFT 微秒级低延迟** 学习笔记，配套原理拆解、可运行源码与工程实践。

---

## 🗺️ HFT 阅读顺序（总纲）

> 详细小节级读/跳指引 → **[HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)**（建议先读此文件）

| 阶段 | 读什么 | 目的 |
|------|--------|------|
| **0** | [Trading-and-Exchanges](./Trading-and-Exchanges/) · LOB/市场结构 | 业务锚点，先懂撮合再写引擎 |
| **1** | [Systems-Performance-2nd](./Systems-Performance-2nd/) | 会量延迟、会排抖动 |
| **2** | [Linux-Kernel-Development-3rd](./Linux-Kernel-Development-3rd/) | 绑核、中断、调度 |
| **3** | [Linux-Virtual-Memory-Manager](./Linux-Virtual-Memory-Manager/) + [Computer-Architecture-6th](./Computer-Architecture-6th/) + [CSAPP-3rd](./CSAPP-3rd/) 内存章 | NUMA / TLB / 伪共享 |
| **4** | [TCP-IP-Illustrated-Vol1](./TCP-IP-Illustrated-Vol1/) → [UNP-Vol1](./UNP-Vol1/) → [CSAPP-3rd](./CSAPP-3rd/) 网络章 → [Linux-Kernel-Networking](./Linux-Kernel-Networking/) | 协议 → API → 内核栈 |
| **5** | [CSAPP-3rd](./CSAPP-3rd/) 优化/并发 + Hennessy 剩余 | 热路径代码优化 |
| **6** | Harris 剩余 + [BPF-Performance-Tools](./BPF-Performance-Tools/) + DPDK 文档 | 业务闭环 + 生产观测 |
| **7** | [HFT-Low-Latency-Practice](./HFT-Low-Latency-Practice/) + [Rust-Quant-Trading-Guide](./Rust-Quant-Trading-Guide/) | 落到 Rust 工程 |

**推荐序号：** 0 → ① → ② → ③ → 外A → 外B → ④ → ⑤ → ⑥ → ⑦ → ⑧ → DPDK → 实战笔记

| 标签 | 含义 |
|------|------|
| 🔴 **必读** | HFT 热路径，不能 skip |
| 🟡 **选读** | 后补或场景触发（如订单走 TCP） |
| ⚪ **跳过** | 本仓库无笔记，默认不读 |

---

## 📚 仓库书籍目录

### 实战笔记（Rust / HFT 工程）

1. **[Rust-Quant-Trading-Guide](./Rust-Quant-Trading-Guide/)** — 11 章，Rust 量化全栈
2. **[HFT-Low-Latency-Practice](./HFT-Low-Latency-Practice/)** — 12 章，低延迟 HFT 实战

### 英文原版笔记（本仓库，8 册）

| # | 文件夹 | 作者 |
|---|--------|------|
| ① | [Systems-Performance-2nd](./Systems-Performance-2nd/) | Brendan Gregg |
| ② | [Linux-Kernel-Development-3rd](./Linux-Kernel-Development-3rd/) | Robert Love |
| ③ | [Linux-Virtual-Memory-Manager](./Linux-Virtual-Memory-Manager/) | Mel Gorman |
| ④ | [Linux-Kernel-Networking](./Linux-Kernel-Networking/) | Rami Rosen |
| ⑤ | [Computer-Architecture-6th](./Computer-Architecture-6th/) | Hennessy & Patterson |
| ⑥ | [CSAPP-3rd](./CSAPP-3rd/) | Bryant & O'Neill |
| ⑦ | [Trading-and-Exchanges](./Trading-and-Exchanges/) | Larry Harris |
| ⑧ | [BPF-Performance-Tools](./BPF-Performance-Tools/) | Brendan Gregg |

### 外部仓库书目（索引，笔记不迁入）

| 外 | 文件夹 | 说明 |
|----|--------|------|
| 外A | [TCP-IP-Illustrated-Vol1](./TCP-IP-Illustrated-Vol1/) | 协议语义 · 笔记在另一仓库 |
| 外B | [UNP-Vol1](./UNP-Vol1/) | Socket API · 笔记在另一仓库 |

每本书文件夹内 **README.md** = 该书必读/选读/跳过速查。

书目裁剪与映射表 → [READING-LIST.md](./READING-LIST.md)

---

## 🛠️ 技术栈

- 主力语言：Rust
- 核心库：RustQuant、Barter-rs、Tokio、io_uring
- 学习辅助：NotebookLM 书籍提炼 + Cursor 代码辅助

## 📌 仓库维护规范

- 根目录直接独立书籍文件夹，零多余嵌套
- 章节按序号独立拆分，单文件单一主题
- 笔记、源码、配图严格分区（`code/`、`assets/`）
- 外部书目只建索引 README，不 duplicate 另一仓库的全文笔记
