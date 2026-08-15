# 第 10 章 · 进程管理

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 9 章 用户态标准库基础](../chapter09_userspace_std_basics/README.md) · 后：[第 11 章 并发编程](../chapter11_concurrency/README.md) · 原书目录：[本书目录 § 第 10 章](../本书目录.md#第-10-章--进程管理)

**本章定位**：**STD 库进程专题**（原书约 p.245+）— **10.1 匿名管道** · **10.2 stdio 重定向** · **10.3 双层 `process`（sys + 公开 API）** · **[10.4 unsafe 分平台走读](./10.4-unsafe-source-walkthrough.md)**。

**原书主线**：IPC 管道 → FD 重定向 → OS 适配层 / 对外 `Command` · **unsafe 收敛到 sys**。

**阅读顺序**：**10.1 → 10.2 → 10.3**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **10.1** | 匿名管道 | [笔记](./10.1-anonymous-pipe.md) |
| **10.2** | 重定向实现分析 | [笔记](./10.2-redirection.md) |
| **10.3** | 进程管理 | [笔记](./10.3-process-mgmt.md) |
| **10.3.1** | OS 相关适配层 | [笔记](./10.3.1-process-os-layer.md) |
| **10.3.2** | 对外接口层 | [笔记](./10.3.2-process-public-api.md) |
| **10.4** | unsafe 源码走读（Unix/Windows） | [笔记](./10.4-unsafe-source-walkthrough.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **10.1** | 匿名管道 | ✅ |
| **10.2** | 重定向实现分析 | ✅ |
| **10.3** | 进程管理 | ✅ |
| **10.3.1** | OS 相关适配层 | ✅ |
| **10.3.2** | 对外接口层 | ✅ |
| **10.4** | unsafe 走读（附录） | ✅ |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| 第 9 章铺垫 | [9.6 进程管理](../chapter09_userspace_std_basics/README.md) |
| `std::process` | [1.3 std 库](../chapter01_std_overview/1.3-std-crate.md) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **10.1～10.2** | 子进程跑风控 / 回放工具、管道传行情 |
| **10.3** | 策略进程 spawn、环境变量与工作目录 |
