# 第 12 章 · 文件系统

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 11 章 并发编程](../chapter11_concurrency/README.md) · 后：[第 13 章 I/O 系统](../chapter13_io/README.md) · 原书目录：[本书目录 § 第 12 章](../本书目录.md#第-12-章--文件系统)

**本章定位**：**`std::path` / `std::fs` / `std::os::*::fs`** — **12.1 sys 适配**（路径编码 · open/readdir）+ **12.2 safe API**（**File Drop** · TOCTOU）+ **[12.4/12.5 源码附录](./12.4-unix-fs-file-walkthrough.md)**。

**原书主线**：OsStr 隔离编码 → 文件/目录 syscall → **`std::fs` 统一接口** · RAII 释 fd。

**阅读顺序**：**[12.0 总览](./12.0-chapter-overview.md) → 12.1 → 12.2**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **12.0** | 本章总览 · 跨平台对照 | [笔记](./12.0-chapter-overview.md) |
| **12.1** | OS 相关适配层 | [笔记](./12.1-fs-os-layer.md) |
| **12.1.1** | 路径名类型分析 | [笔记](./12.1.1-path-types.md) |
| **12.1.2** | 普通文件操作分析 | [笔记](./12.1.2-file-ops.md) |
| **12.1.3** | 目录操作分析 | [笔记](./12.1.3-dir-ops.md) |
| **12.2** | 对外接口层 | [笔记](./12.2-fs-public-api.md) |
| **12.4** | Unix File 走读（附录） | [笔记](./12.4-unix-fs-file-walkthrough.md) |
| **12.5** | Path 编码走读（附录） | [笔记](./12.5-path-encoding-walkthrough.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **12.0** | 本章总览 | ✅ |
| **12.1** | OS 相关适配层 | ✅ |
| **12.1.1** | 路径名类型分析 | ✅ |
| **12.1.2** | 普通文件操作分析 | ✅ |
| **12.1.3** | 目录操作分析 | ✅ |
| **12.2** | 对外接口层 | ✅ |
| **12.4** | Unix File 走读 | ✅ |
| **12.5** | Path 编码走读 | ✅ |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| fd / 所有权 | [9.5 文件描述符](../chapter09_userspace_std_basics/README.md) |
| `Path` / `OsString` | [9.1.4 `OsString`](../chapter09_userspace_std_basics/9.1.4-osstring.md) |
| 进程 fd / 管道 | [第 10 章](../chapter10_process_management/README.md) |
| I/O trait | [第 13 章 I/O](../chapter13_io/README.md) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **12.1.2** | 落盘日志、tick 回放、`mmap` 前置 open |
| **12.1.1** | 跨平台路径（Windows / Linux 部署） |
| **12.2 TOCTOU** | 多进程写同一配置/锁文件时用 **`create_new`** |
