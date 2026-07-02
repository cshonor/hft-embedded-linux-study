## 6. 全路径串联与后续章节索引

> Ch 12–16 的 **端到端** read 路径

---

### 一、规范 read 全链路

```
用户 read(fd, buf, n)
    ↓ Ch 10 syscall
VFS sys_read → file->f_op->read（或 aio_read）
    ↓ Ch 12
generic_file_read()
    ↓ Ch 15
address_space 基数树查页
    ├─ 命中 → copy_to_user
    └─ 未命中 → readpage / 预读
            ↓ Ch 14
            bio → request_queue → 驱动 → DMA
            ↓ IRQ 完成
            页入缓存 → copy_to_user
```

**write** 路径对称：copy_from_user → 脏页 →（延迟）writepage → bio。

---

### 二、模式选择速查

| 需求 | 模式 |
|------|------|
| 通用文件 I/O | 默认 read/write + 页缓存 |
| 强持久化每次写 | `O_SYNC` 或 write + `fsync` |
| 大文件、指针式访问 | `mmap` + demand paging |
| DB 自管缓存 | `O_DIRECT` + 自对齐 buffer |
| 重叠 I/O | AIO / modern **io_uring** |

---

### 三、后续章节索引

| Ch 16 主题 | 继续读 |
|------------|--------|
| 页回收、cache 压力 | [Ch 17 页框回收](../chapter-17-page-reclaim/) 🟡 |
| Ext2 等具体 FS | [Ch 18 Ext2/Ext3](../chapter-18-ext2-ext3/) ⚪ |
| VFS 对象 | [Ch 12 VFS](../chapter-12-VFS/) ⚪ |
| 页缓存 / 写回 | [Ch 15](../chapter-15-page-cache/) ⚪ |
| 用户态 I/O | [08 TLPI](../../../06-The-Linux-Programming-Interface/) |
| O_DIRECT / io_uring | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. AIO](./section-5-异步IO.md) · 下一章 [Ch 17 页框回收](../chapter-17-page-reclaim/)
