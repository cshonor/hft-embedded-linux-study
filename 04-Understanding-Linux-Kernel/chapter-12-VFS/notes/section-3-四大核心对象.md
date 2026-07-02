## 3. VFS 四大核心对象

> 通用文件模型 — **superblock / inode / file / dentry**

---

### 一、超级块对象 (Superblock)

| 含义 | 一个 **已挂载** 的文件系统实例 |
|------|-------------------------------|
| 存储 | 块大小、最大文件长度、FS 类型等 **全局信息** |
| 方法 | `write_super`、`alloc_inode` 等 **文件系统级** 操作 |

例：根分区、`/proc` 各对应一个 superblock。

---

### 二、索引节点对象 (Inode)

| 含义 | 一个 **具体文件** 的元数据 |
|------|---------------------------|
| 内容 | 大小、uid/gid、权限、时间戳 … |
| 唯一性 | 文件存在期间 inode **唯一**（同 FS 内由 inode 号标识） |

**不含文件名** — 文件名在 dentry 中。

---

### 三、文件对象 (File)

| 含义 | **进程 ↔ 文件** 的一次打开交互 |
|------|-------------------------------|
| 状态 | 当前 **文件偏移**、`O_RDONLY` / `O_APPEND` 等 flags |
| 关系 | 一个 inode 可被 **多进程** 各自打开 → **多个 file 对象** |

`open()` 创建 file；`read`/`write` 主要操作 file + inode。

---

### 四、目录项对象 (Dentry)

| 含义 | 路径中的 **一个分量**（目录项） |
|------|--------------------------------|
| 作用 | 将 **文件名** 链接到 **inode** |
| 例 | `/tmp/test` → dentry：`/`、`tmp`、`test` |

Dentry 构成 **目录树** 的内存视图；与 inode 多对一（硬链接共享 inode）。

---

### 五、对象关系简图

```
superblock（整个 FS）
    ↓
inode（文件元数据）←—— dentry（名字 → inode）
    ↑
file（某进程的打开实例：offset、flags）
```

→ 同步保护 inode：[Ch 5 section-7](../../chapter-05-kernel-synchronization/notes/section-7-选型与实例.md)

> **深潜可选：** `super_operations`、`inode_operations`、`file_operations` 函数指针表 — 即 VFS **多态** 的实现。

---

← [2. VFS 作用](./section-2-VFS的作用.md) · 下一节 [4. 高速缓存](./section-4-高速缓存.md)
