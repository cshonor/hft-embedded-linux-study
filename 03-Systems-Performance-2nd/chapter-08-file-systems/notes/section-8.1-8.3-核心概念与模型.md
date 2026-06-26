## 8.1–8.3 核心概念与模型

### 逻辑 I/O vs 物理 I/O

```
Application                    File System                    Disk
    |  read()/write()              |                              |
    | -------- 逻辑 I/O ---------> |                              |
    |                              | ---- 物理 I/O（可能 0 次）--> |
    |                              |     （cache 命中则不发盘）      |
```

| 效应 | 含义 | 例子 |
|------|------|------|
| **I/O 通货紧缩** | 多次逻辑读 → 一次物理读 | page cache 命中、read-ahead |
| **I/O 通货膨胀** | 少量逻辑写 → 多次物理写 | 元数据 journal、小写放大 |
| **合并** | 多次逻辑写 → 一次物理写 | write-back 合并 |

**HFT 诊断：**

- 「磁盘很忙」→ 先 **`cachestat`** / **`filetop`**：是 **真刷盘** 还是 **page cache 在涨**？
- 「读配置慢一次」→ 冷 cache + read-ahead 预热；**生产应启动时预热**。

→ Ch 7 [file paging vs swap](../../chapter-07-memory/)

### 缓存与缓冲

| 机制 | 行为 | 延迟 |
|------|------|------|
| **Read cache (page cache)** | 读过进内存，再读命中 | 命中 ≈ 内存速度 |
| **Write-back buffer** | 写先进 cache，标记 dirty，后台 flush | 写返回快；**掉电丢数据** 风险 |
| **Write-through** | 写同时落盘 | 慢，一致性强 |

**Gregg 观点：** FS 性能 **往往比裸盘对应用更重要** — 因为绝大多数逻辑 I/O 被 cache 吸收。

**HFT：**

- **行情 tick 路径**：不应依赖 FS read；应用内 / mmap 预热 / 共享内存。
- **异步审计日志**：write-back 友好；关键 checkpoint 才 **`fsync`** — 并预期 latency spike。

### 预取（Read-Ahead / Prefetch）

- 检测到 **顺序读** → 内核提前读后续页进 cache。
- 随机读 → 预取可能 **浪费 I/O** 且污染 cache。

```c
posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);  // 提示顺序读
posix_fadvise(fd, off, len, POSIX_FADV_DONTNEED); // 提示可丢弃 cache
```

**HFT replay：** 顺序读历史 tick 文件时开 SEQUENTIAL；replay 完 DONTNEED 释放 cache 给 order book。

### 绕过文件系统机制

| 方式 | API / 标志 | 特点 |
|------|------------|------|
| **Direct I/O** | `O_DIRECT` | 对齐 buffer；**绕过 page cache**；自管缓存 |
| **mmap** | `mmap()` | 文件 ↔ 虚拟地址；缺页 = page fault；少 `read()` syscall |

| | 适用 | 不适用 |
|---|------|--------|
| **O_DIRECT** | DB、时序库、自研 WAL | 小随机读、依赖 kernel read-ahead |
| **mmap** | 大只读数据集、共享映射 | 需精确控制 fault 时序的热路径 |

→ [01-CSAPP Ch9 mmap](../../../01-CSAPP-3rd/chapter-09-virtual-memory/) · Ch 5 [I/O 与缓冲](../../chapter-05-applications/)

### 元数据（Metadata）

| 类型 | 内容 | I/O 特点 |
|------|------|----------|
| **逻辑元数据** | 权限、时间戳、目录名 | 每次 `open`/`stat`/`readdir` 可能触发 |
| **物理元数据** | inode、bitmap、journal | 小随机写，**journal 放大** |

**经典坑：** 默认 **`atime` 更新** — 每次读文件都写 inode → 无谓写 I/O。现代默认 **`relatime`**；极端可 **`noatime`** 挂载。

---


---

← [本章导读](../README.md)
