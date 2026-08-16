## 1. 本章定位

> **ULK Ch 18 Ext2 and Ext3** · VFS 抽象 **落地到磁盘布局**

---

### 一、本章讲什么

Ch 12 VFS 讲 **通用对象**；本章讲 **Ext2/Ext3 在磁盘上长什么样**、内核如何缓存与管理：

| 主题 | 要点 |
|------|------|
| **块组** | 超级块、位图、inode 表 |
| **寻址** | `i_block[15]` 直接/间接块 — 类似 **多级页表** |
| **空洞与分配** | 不分配全零块、预分配减碎片 |
| **内存镜像** | `ext2_sb_info`、`ext2_inode_info` |
| **Ext3** | JBD 日志、三种 journal 模式 |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-Ext2磁盘数据结构.md) | 块组、superblock、bitmap、inode |
| [3](./section-3-数据块寻址.md) | 直接 / 一/二/三次间接 |
| [4](./section-4-空洞与块分配.md) | file holes、预分配 |
| [5](./section-5-Ext2内存数据结构.md) | 磁盘结构在 RAM 中的副本 |
| [6](./section-6-Ext3与日志机制.md) | JBD、transaction、journal/ordered/writeback |

---

### 三、在 Linux 链上的位置

```
Ch 12 VFS inode / superblock 抽象
Ch 14–16 块 I/O、页缓存、read 路径
Ch 18 Ext2/Ext3 具体 on-disk 布局（本章）
Ch 19 IPC（进程通信）
```

HFT：生产环境多用 **xfs/ext4** 或 **裸设备**；Ext2/Ext3 仍有助于理解 **块组、inode、日志** 等通用 FS 概念。

---

← [Ch 18 导读](../README.md) · 下一节 [2. 磁盘结构](./section-2-Ext2磁盘数据结构.md)
