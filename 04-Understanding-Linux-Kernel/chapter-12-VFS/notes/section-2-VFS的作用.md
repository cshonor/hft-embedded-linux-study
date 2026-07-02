## 2. VFS 的作用 (The Role of VFS)

---

### 一、抽象接口

| 用户视角 | VFS 职责 |
|----------|----------|
| 同一套 POSIX API | `open()`, `read()`, `write()`, `lseek()`, `close()` … |
| 多种底层 FS | Ext2、FAT、NFS、procfs … **无需改应用** |

应用 **不知道** 文件在磁盘上的物理布局 — 只与 VFS 对象交互。

→ syscall 入口：[Ch 10](../chapter-10-system-calls/) · 用户态：[08 TLPI](../../../06-The-Linux-Programming-Interface/)

---

### 二、纯内存 vs 必须落盘

VFS **并非** 每次操作都碰磁盘：

| 操作 | 典型处理 |
|------|----------|
| **`lseek()`** | 常仅改 **file 对象** 中的偏移量 — **内存即可** |
| **`read()` / `write()`** | 可能命中 **页缓存**；未命中才调具体 FS |
| **元数据变更** | 经具体 FS 的 superblock/inode 方法写回 |

**分层：** VFS 做通用语义 → 具体 FS 做磁盘布局。

---

### 三、面向对象的 C 实现

VFS 用 **结构体 + 函数指针表** 模拟 OOP 多态（非 C++）：

```
应用 syscall
    ↓
VFS 通用层（file_operations 等）
    ↓ 函数指针
ext2_* / nfs_* / proc_* …
```

→ 四大对象详述：[section-3](./section-3-四大核心对象.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 核心对象](./section-3-四大核心对象.md)
