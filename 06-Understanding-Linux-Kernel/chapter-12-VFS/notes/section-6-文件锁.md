## 6. 文件锁 (File Locking)

> 多进程 **并发访问同一文件** 的同步

---

### 一、建议性 vs 强制性

| 类型 | 行为 |
|------|------|
| **建议性锁 (Advisory)** | 仅 **合作进程** 自觉检查锁；不检查者仍可读写 |
| **强制性锁 (Mandatory)** | 内核在 **`read`/`write`** 时 **强制检查** — 未持锁者被阻塞 |

Linux **默认以建议性为主**；强制性需 FS 与挂载选项配合，使用较少。

---

### 二、两种接口

| 接口 | 系统调用 | 粒度 |
|------|----------|------|
| **BSD 风格** | **`flock()`** | 通常 **整个文件** |
| **POSIX 风格** | **`fcntl()`** | **字节范围**；共享读锁 / 排他写锁 |

VFS 提供 **统一锁层**，具体 FS 可挂钩实现。

→ syscall：[Ch 10](../chapter-10-system-calls/) · 用户态：[08 TLPI](../../../08-The-Linux-Programming-Interface/)

---

### 三、本章小结

```
POSIX open/read/write
    ↓
VFS 对象（sb / inode / file / dentry）+ 缓存
    ↓ path_lookup
具体 FS 或 procfs/sysfs
    ↓
页缓存 / 块 I/O（→ Ch 16）
```

---

### 四、后续章节索引

| Ch 12 主题 | 继续读 |
|------------|--------|
| read/write 完整路径 | [Ch 16 文件访问](../chapter-16-file-access.md) ⚪ |
| Ext2 具体实现 | [Ch 17 Ext2](../chapter-17-ext2.md) ⚪ |
| 块 I/O | [Ch 13 I/O 架构](../chapter-13-io-architecture/) ⚪ |
| Unix FS 概念 | [Ch 1 section-4](../chapter-01-introduction/notes/section-4-Unix文件系统概述.md) |
| LKD 对照 | Linux Kernel Development Ch 13 |
| inode 锁实例 | [Ch 5 section-7](../chapter-05-kernel-synchronization/notes/section-7-选型与实例.md) |

---

← [5. 挂载与路径查找](./section-5-挂载与路径查找.md) · 下一章 [Ch 13 I/O 架构](../chapter-13-io-architecture/)
