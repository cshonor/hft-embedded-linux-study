## 5. Ext2 内存数据结构 (Memory Data Structures)

> 挂载时 — 磁盘结构 **读入 RAM 并缓存**

---

### 一、为何需要内存副本

磁盘访问 **慢** — 内核维护 **内存中的 FS 私有结构**，与 VFS 对象 **嵌套**。

---

### 二、主要结构

| 磁盘 | 内存（Ext2 专有） | VFS 包装 |
|------|-------------------|----------|
| **`ext2_super_block`** | **`ext2_sb_info`** | VFS **`super_block`** |
| **磁盘 inode** | **`ext2_inode_info`** | VFS **`inode`** |

打开 / 访问文件时：

- 磁盘 inode → 填入 **`ext2_inode_info` + 通用 inode**  
- 超级块信息 → **`ext2_sb_info`** — 块组位图指针、挂载选项等  

→ inode 缓存：[Ch 12 section-4](../chapter-12-VFS/notes/section-4-高速缓存.md)

---

### 三、与 read/write 的衔接

VFS **`address_space`** 挂在 inode 上 — 文件 **页** 在页缓存；**元数据** 在 ext2 inode 内存副本。

→ [Ch 16 generic_file_read](../chapter-16-file-access/notes/section-3-读写与预读.md)

---

← [4. 空洞与分配](./section-4-空洞与块分配.md) · 下一节 [6. Ext3 日志](./section-6-Ext3与日志机制.md)
