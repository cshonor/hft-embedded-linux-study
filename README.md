# rust-quant-hft-handbook

本仓库收录 **Rust 全栈量化** + **HFT 微秒级低延迟** 学习笔记，配套原理拆解、可运行源码与工程实践。

---

## 🗺️ HFT 阅读顺序（总纲）

> 详细小节级读/跳指引 → **[HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)**（建议先读此文件）

| 阶段 | 读什么 | 目的 |
|------|--------|------|
| **0** | [00-Trading-and-Exchanges](./00-Trading-and-Exchanges/) · LOB/市场结构 | 业务锚点，先懂撮合再写引擎 |
| **1** | [01-Systems-Performance-2nd](./01-Systems-Performance-2nd/) | 会量延迟、会排抖动 |
| **2** | [02-Linux-Kernel-Development-3rd](./02-Linux-Kernel-Development-3rd/) | 绑核、中断、调度 |
| **3** | [03-Linux-Virtual-Memory-Manager](./03-Linux-Virtual-Memory-Manager/) + [07-Computer-Architecture-6th](./07-Computer-Architecture-6th/) + [08-CSAPP-3rd](./08-CSAPP-3rd/) 内存章 | NUMA / TLB / 伪共享 |
| **4** | [04-TCP-IP-Illustrated-Vol1](./04-TCP-IP-Illustrated-Vol1/) → [05-UNP-Vol1](./05-UNP-Vol1/) → [08-CSAPP-3rd](./08-CSAPP-3rd/) 网络章 → [06-Linux-Kernel-Networking](./06-Linux-Kernel-Networking/) | 协议 → API → 内核栈 |
| **5** | [08-CSAPP-3rd](./08-CSAPP-3rd/) 优化/并发 + Hennessy 剩余 | 热路径代码优化 |
| **6** | Harris 剩余 + [09-BPF-Performance-Tools](./09-BPF-Performance-Tools/) + DPDK 文档 | 业务闭环 + 生产观测 |
| **7** | [10-HFT-Low-Latency-Practice](./10-HFT-Low-Latency-Practice/) + [11-Rust-Quant-Trading-Guide](./11-Rust-Quant-Trading-Guide/) | 落到 Rust 工程 |

**推荐序号：** 0 → ① → ② → ③ → 外A → 外B → ④ → ⑤ → ⑥ → ⑦ → ⑧ → DPDK → 实战笔记

| 标签 | 含义 |
|------|------|
| 🔴 **必读** | HFT 热路径，不能 skip |
| 🟡 **选读** | 后补或场景触发（如订单走 TCP） |
| ⚪ **跳过** | 本仓库无笔记，默认不读 |

---

## 📚 仓库目录（按文件夹序号 = 阅读顺序）

| 序号 | 文件夹 | 内容 |
|------|--------|------|
| 00 | [00-Trading-and-Exchanges](./00-Trading-and-Exchanges/) | Harris · 市场/LOB（**29 章**，[目录](./00-Trading-and-Exchanges/OUTLINE.md)） |
| 01 | [01-Systems-Performance-2nd](./01-Systems-Performance-2nd/) | Gregg · 性能方法论 |
| 02 | [02-Linux-Kernel-Development-3rd](./02-Linux-Kernel-Development-3rd/) | Love · 内核调度/中断 |
| 03 | [03-Linux-Virtual-Memory-Manager](./03-Linux-Virtual-Memory-Manager/) | Gorman · 虚拟内存 |
| 04 | [04-TCP-IP-Illustrated-Vol1](./04-TCP-IP-Illustrated-Vol1/) | Stevens · 协议 → [Computer-Networking](https://github.com/cshonor/Computer-Networking/tree/main/TCP-IP-Volume1-Protocols) |
| 05 | [05-UNP-Vol1](./05-UNP-Vol1/) | Stevens · Socket API → [Computer-Networking](https://github.com/cshonor/Computer-Networking/tree/main/UNP_Vol1) |
| 06 | [06-Linux-Kernel-Networking](./06-Linux-Kernel-Networking/) | Rosen · 内核网络栈 |
| 07 | [07-Computer-Architecture-6th](./07-Computer-Architecture-6th/) | Hennessy · 硬件/缓存 |
| 08 | [08-CSAPP-3rd](./08-CSAPP-3rd/) | CSAPP · 12 章 + 附录 [OUTLINE](./08-CSAPP-3rd/OUTLINE.md) |
| 09 | [09-BPF-Performance-Tools](./09-BPF-Performance-Tools/) | Gregg · eBPF 观测 |
| 10 | [10-HFT-Low-Latency-Practice](./10-HFT-Low-Latency-Practice/) | HFT 实战笔记 · 12 章 |
| 11 | [11-Rust-Quant-Trading-Guide](./11-Rust-Quant-Trading-Guide/) | Rust 量化工程 · 11 章 |

每本书文件夹内 **README.md** = 必读/选读/跳过速查。

书目裁剪与映射表 → [READING-LIST.md](./READING-LIST.md)

---

## 🛠️ 技术栈

- 主力语言：Rust
- 核心库：RustQuant、Barter-rs、Tokio、io_uring
- 学习辅助：NotebookLM 书籍提炼 + Cursor 代码辅助

## 📌 仓库维护规范

- 根目录文件夹以 **`00-` ~ `11-` 序号前缀** 排序，序号 = HFT 推荐阅读先后
- 章节按序号独立拆分，单文件单一主题
- 笔记、源码、配图严格分区（`code/`、`assets/`）
- 外部书目只建索引 README，不 duplicate 另一仓库的全文笔记
