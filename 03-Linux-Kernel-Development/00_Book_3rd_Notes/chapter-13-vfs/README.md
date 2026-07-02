# Ch 13 虚拟文件系统 · The Virtual Filesystem

> **Linux Kernel Development 3rd** · Robert Love · **背景**

> 本章定位：**VFS 抽象层** — 四大对象、`file_operations`、**dcache**、挂载与进程 fd 表。统一 **open/read/write** 如何落到 ext4/FAT/管道/设备。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 抽象接口** | VFS 粘合剂 | syscall ↔ 具体 FS |
| **② 四大对象** | superblock/inode/dentry/file | 面向对象 C |
| **③ 操作表** | `*_operations` | 函数指针多态 |
| **④ dcache** | 目录项缓存 | 路径解析加速 |
| **⑤ FS 数据结构** | `file_system_type` · `vfsmount` | 注册与挂载 |
| **⑥ 进程相关** | `files_struct` 等 | fd · 命名空间 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 通用文件系统接口与抽象层 | [notes/section-13.1-通用文件系统接口与抽象层.md](./notes/section-13.1-通用文件系统接口与抽象层.md) |
| VFS 的面向对象设计 | [notes/section-13.2-VFS-的面向对象设计.md](./notes/section-13.2-VFS-的面向对象设计.md) |
| 操作对象 | [notes/section-13.3-操作对象.md](./notes/section-13.3-操作对象.md) |
| 目录项缓存 | [notes/section-13.4-目录项缓存.md](./notes/section-13.4-目录项缓存.md) |
| 与文件系统相关的数据结构 | [notes/section-13.5-与文件系统相关的数据结构.md](./notes/section-13.5-与文件系统相关的数据结构.md) |
| 与进程相关的数据结构 | [notes/section-13.6-与进程相关的数据结构.md](./notes/section-13.6-与进程相关的数据结构.md) |
| 从 open 到 read（概念路径） | [notes/section-13.7-从-open-到-read概念路径.md](./notes/section-13.7-从-open-到-read概念路径.md) |

---

## 本章小结

| 对象 | 一句话 |
|------|--------|
| **superblock** | 已挂载 FS 实例 |
| **inode** | 文件元数据 |
| **dentry** | 路径名组件 + **dcache** |
| **file** | 进程打开实例 + 偏移 |
| **`*_operations`** | 具体 FS/驱动实现 |
| **`files_struct`** | **fd 表** |

---

## 本章学习目标 · 自检

- [ ] 画出 **inode vs dentry vs file** 分工
- [ ] 说出 **`file_operations`** 在 VFS 多态中的作用
- [ ] 解释 **dcache 负缓存** 用途
- [ ] 区分 **`file_system_type`** 与 **`vfsmount`**
- [ ] 知 **`files_struct` / `fs_struct` / namespace** 各管什么
- [ ] HFT：热路径 **长生命 fd + mmap** vs 冷路径反复 `open`

---

## 相关章节

- 上一章：[../chapter-12-memory-management/](../chapter-12-memory-management/)
- 下一章：[../chapter-14-block-io/](../chapter-14-block-io/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
