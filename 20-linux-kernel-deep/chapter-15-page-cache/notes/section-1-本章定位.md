## 1. 本章定位

> **ULK Ch 15 The Page Cache** · 用 RAM **缓存磁盘数据**

---

### 一、本章讲什么

**页高速缓存 (Page Cache)** 是 Linux **主要磁盘缓存** — 几乎所有文件读写都经它：

| 主题 | 要点 |
|------|------|
| **读** | 先查缓存；命中则 **零磁盘** |
| **写** | 先改缓存页 → 标 **脏** → **延迟** 写回 |
| **组织** | `address_space` + **基数树** 索引页 |
| **回写** | `pdflush` 内核线程池 |
| **持久化** | `sync` / `fsync` / `fdatasync` |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-页缓存与address_space.md) | 页缓存、`address_space`、`a_ops` |
| [3](./section-3-基数树与标签.md) | Radix Tree、DIRTY/WRITEBACK 标签 |
| [4](./section-4-缓冲页与buffer_head.md) | Buffer Page、`buffer_head` |
| [5](./section-5-回写脏页与pdflush.md) | 延迟写、`pdflush` 唤醒条件 |
| [6](./section-6-同步系统调用.md) | sync / fsync / fdatasync |

---

### 三、在 Linux 链上的位置

```
Ch 8  struct page（物理页）
Ch 9  缺页、文件 mmap
Ch 12 inode / VFS
Ch 14 bio 块 I/O（缓存未命中时）
Ch 15 页缓存（本章）
Ch 16 文件 read/write 完整路径
Ch 17 内存紧张时回收缓存页
```

HFT：热路径应 **避免冷 cache 读** 与 **`fsync` 尾延迟**；日志持久化需理解脏页与 sync 语义。

---

← [Ch 15 导读](../README.md) · 下一节 [2. address_space](./section-2-页缓存与address_space.md)
