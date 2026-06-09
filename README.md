# rust-quant-hft-handbook

本仓库收录两套深度量化交易学习笔记，聚焦 Rust 高性能全栈量化 + 微秒级高频低延迟系统开发，配套完整原理拆解、可运行源码与工程落地实践。

## 📚 仓库书籍目录
1.  **Rust-Quant-Trading-Guide**
    全书共11章，从零搭建完整 Rust 量化交易系统
    覆盖：Rust工程、数据处理、策略开发、回测、实盘对接、风控、生产部署

2.  **HFT-Low-Latency-Practice**
    高频交易系统开发实战
    覆盖：硬件优化、内核旁路、无锁编程、订单簿、低延迟网络、HFT专属策略

## 📖 HFT 英文原版笔记（8 册，按阅读顺序）

| # | 文件夹 | 作者 | 精读章节数 |
|---|--------|------|-----------|
| 1 | [Systems-Performance-2nd](./Systems-Performance-2nd/) | Brendan Gregg | 5 |
| 2 | [Linux-Kernel-Development-3rd](./Linux-Kernel-Development-3rd/) | Robert Love | 7 |
| 3 | [Linux-Virtual-Memory-Manager](./Linux-Virtual-Memory-Manager/) | Mel Gorman | 5 |
| 4 | [Linux-Kernel-Networking](./Linux-Kernel-Networking/) | Rami Rosen | 7 |
| 5 | [Computer-Architecture-6th](./Computer-Architecture-6th/) | Hennessy & Patterson | 4 |
| 6 | [CSAPP-3rd](./CSAPP-3rd/) | Bryant & O'Neill | 9 |
| 7 | [Trading-and-Exchanges](./Trading-and-Exchanges/) | Larry Harris | 4 |
| 8 | [BPF-Performance-Tools](./BPF-Performance-Tools/) | Brendan Gregg | 5 |

章节裁剪说明、DPDK/RDMA 官方文档、与 `HFT-Low-Latency-Practice` 映射表 → 见 [READING-LIST.md](./READING-LIST.md)

## 🛠️ 技术栈
- 主力语言：Rust
- 核心库：RustQuant、Barter-rs、Tokio、io_uring
- 学习辅助：NotebookLM 书籍提炼 + Cursor 代码辅助

## 📌 仓库维护规范
- 根目录直接独立书籍文件夹，零多余嵌套
- 章节按序号独立拆分，单文件单一主题，轻量化查阅
- 笔记、源码、配图严格分区存放
- 后续新增量化相关书籍，直接新建同级顶级目录
