## 1. 本章定位

> **ULK Ch 16 Accessing Files** · 文件读写 **如何真正发生**

---

### 一、本章讲什么

前几章的抽象在此 **汇合**：

| 前置章 | 提供 |
|--------|------|
| **Ch 12 VFS** | open / read / write / mmap 入口 |
| **Ch 15 页缓存** | `address_space`、脏页 |
| **Ch 14 块层** | `bio`、I/O 调度 |
| **Ch 9** | mmap 缺页、VMA |

本章讲 **五种访问模式** 及 **`generic_file_read/write`、预读、mmap、AIO** 的实际路径。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-文件访问模式.md) | 规范 / 同步 / mmap / O_DIRECT / AIO |
| [3](./section-3-读写与预读.md) | generic_file_*、read-ahead |
| [4](./section-4-内存映射.md) | demand paging、非线性 mmap |
| [5](./section-5-异步IO.md) | kiocb、io_setup / io_submit |
| [6](./section-6-全路径串联与索引.md) | 端到端路径 + 后续章节 |

---

### 三、HFT 视角

| 模式 | 典型用途 |
|------|----------|
| **页缓存 + read** | 配置文件、历史数据 — 注意 **预读与冷 cache** |
| **O_DIRECT** | 数据库式自管缓存 — **绕过** 页缓存 |
| **mmap** | 大文件、共享只读数据集 |
| **AIO** | 重叠 I/O 与计算（ULK 2.6 原生 AIO 较有限；modern 有 io_uring） |

---

← [Ch 16 导读](../README.md) · 下一节 [2. 访问模式](./section-2-文件访问模式.md)
