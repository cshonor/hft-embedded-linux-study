# 2. 文件系统基础 (Background)

### 逻辑 I/O vs 物理 I/O

| 类型 | 说明 |
|------|------|
| **逻辑 I/O** | 应用 `read`/`write`/`mmap` 对 **文件** 的请求 |
| **物理 I/O** | 真正下到块设备的 I/O |

**页缓存命中** 时，逻辑读 **不** 产生物理读 — 延迟在内存量级。

### 三大缓存

| 缓存 | 作用 | 占用 |
|------|------|------|
| **Page Cache** | 文件内容与 I/O 缓冲 | 通常 **最大** |
| **Inode Cache** | inode 元数据、权限 | 中等 |
| **dcache** | 路径名 → inode 映射 | 加速路径查找 |

→ 内核实现：[05-LKD VFS](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-13-vfs/) · [06-ULK](../06-Understanding-Linux-Kernel/) · [01-30days-os Day 18–19 FAT](../09-system-low-level-hands-on/01-30days-os/day-18-dir/)

### 预读与写回

| 机制 | 行为 |
|------|------|
| **Read-ahead** | 检测顺序读 → **提前** 读入后续页到 cache |
| **Write-back** | 脏页驻留内存 → **异步** 刷盘，避免阻塞写路径 |

**调优陷阱：** 预读过激 → 浪费内存与 I/O（`readahead` 工具可见 **预读但未使用** 的页）。

---
