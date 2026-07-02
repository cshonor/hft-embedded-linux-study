## 6. 同步系统调用 (sync / fsync / fdatasync)

> 用户态 **强制** 持久化 — 绕过纯延迟写

---

### 一、三个 syscall 对比

| 调用 | 范围 | 行为 |
|------|------|------|
| **`sync()`** | **全局** | 刷新系统中 **所有** 脏缓冲区到磁盘（粗粒度、可能很重） |
| **`fsync(fd)`** | **单个打开文件** | 该文件 **数据块 + inode 元数据** 均落盘 |
| **`fdatasync(fd)`** | **单个打开文件** | 仅 **数据块** — **不** 强制 inode 块（除非 FS 需 inode 才能读数据） |

→ syscall 路径：[Ch 10](../chapter-10-system-calls/) · TLPI

---

### 二、典型使用场景

| 场景 | 选择 |
|------|------|
| 数据库 **事务提交** | **`fsync`** / **`fdatasync`** 保证 WAL 持久 |
| 关机 / 卸载前 | **`sync`** |
| 仅需数据、可接受部分元数据延迟 | **`fdatasync`**（可能更快） |

HFT：**日志/fsync 延迟** 是 tail latency 常见来源 — 专用盘、顺序写、减少 sync 频率。

---

### 三、本章小结

```
read  → 查 address_space 基数树 → 命中 / readpage → 页缓存
write → 改页 → dirty 标签
    ↓ 后台 pdflush 或 sync/fsync
writepage → bio → Ch 14 块层 → 磁盘
```

---

### 四、后续章节索引

| Ch 15 主题 | 继续读 |
|------------|--------|
| read/write 完整路径 | [Ch 16 文件访问](../chapter-16-file-access/) ⚪ |
| 回收缓存页 | [Ch 17 页回收](../chapter-17-page-reclaim.md) 🟡 |
| bio / 块层 | [Ch 14 块设备](../chapter-14-block-devices/) ⚪ |
| inode / VFS | [Ch 12 VFS](../chapter-12-VFS/) ⚪ |
| 07 Gorman | [页缓存 / 写回](../../../05-Linux-Virtual-Memory-Manager/) |

---

← [5. 脏页回写](./section-5-回写脏页与pdflush.md) · 下一章 [Ch 16 文件访问](../chapter-16-file-access/)
