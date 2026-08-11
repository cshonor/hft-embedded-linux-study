# TLPI 第 14 章 — File Systems

**优先级**：🔴（路径解析、挂载、硬链接、持久化与 fsck/日志理解）  
**前置**：[Ch13 File I/O Buffering](../chapter-13-file-io-buffering/notes.md)  
**后置**：[Ch15 File Attributes](../chapter-15-file-attributes/notes.md) · [Ch18 目录与链接](../chapter-18-directories-links/notes.md) · [Ch19 inotify](../chapter-19-monitoring-file-events/notes.md)

---

## 小节目录

- [14.1 设备特殊文件](./notes/14.1-section-14-1.md)
- [14.2 磁盘与分区](./notes/14.2-section-14-2.md)
- [14.3 传统布局（ext2 范例）](./notes/14.3-ext2.md)
- [14.4 i-node（核心）](./notes/14.4-node.md)
- [14.5 VFS（Virtual File System）](./notes/14.5-vfs-virtual-file.md)
- [14.6 日志 FS 与内存 FS](./notes/14.6-fs-fs.md)
- [14.7 挂载点与单一目录树](./notes/14.7-directory.md)
- [14.8 `mount` / `umount`（需特权，如 `CAP_SYS_ADMIN`）](./notes/14.8-mount-umount-capsysadmin.md)
- [14.9 文件系统信息 API](./notes/14.9-api.md)

---

## 章节目标


理解分区与传统 FS 布局（以 ext2 为范例）、**inode** 与路径解析；掌握 **VFS** 抽象；区分磁盘 FS / tmpfs / 日志 FS；会 `mount`/`umount` 与 `statvfs`；打通硬件 ↔ VFS ↔ 应用。

---


---

## 14.10 速查：`statfs` vs `statvfs` · 挂载标志


| | `statfs` | `statvfs` |
|--|----------|-----------|
| 可移植 | Linux 等扩展 | **POSIX/SUSv3** |
| 结构体 | `struct statfs` | `struct statvfs` |
| 建议 | Linux 专用够用 | **跨平台首选** |

| flag | 作用一句话 |
|------|------------|
| `MS_RDONLY` | 只读挂载 |
| `MS_NOEXEC` | 不能在此执行程序 |
| `MS_NOSUID` | setuid 位无效 |
| `MS_REMOUNT` | 原地改选项 |
| `MS_BIND` | 绑定镜像目录树 |
| `MNT_DETACH` | 懒卸载 |
| `MNT_FORCE` | 强制卸载（危险） |

---


---

## 14.11 易错清单


1. inode 号仅在**同一 FS** 内可比；硬链接不能跨 FS  
2. `unlink`：链接计数→0 且无打开者 → 才真正释放数据块  
3. 挂载遮盖：别把重要目录当挂载点却忘了「底下有东西」  
4. tmpfs 吃内存/swap，不占普通磁盘配额语义  
5. dentry 是**内核缓存**，不落盘  
6. 日志 ≠ 用户数据已持久化；关键写仍要 `fsync`/`fdatasync`  
7. 可移植：`statvfs`；挂载信息可解析 `/proc/mounts`  

---


---

## 练习


1. `statvfs` 简易 `df`  
2. root 下 `MS_BIND` 绑定挂载再卸载  
3. 同分区硬链接 inode 相同；跨分区对比 inode  
4. （选）`MS_RDONLY` / `MS_NOEXEC` 行为  
5. 解析 `/proc/mounts`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 文件名在目录块；元数据在 inode |
| 2 | inode# 仅同 FS 有效；硬链接不跨分区 |
| 3 | VFS：super_block / inode / dentry / file |
| 4 | 单一目录树 + 挂载点（遮盖原内容） |
| 5 | 可移植查容量用 `statvfs`；`df` 同源 |
| 6 | 日志保元数据恢复速度；数据落盘仍靠 fsync |

---


---

## 参考


- Kerrisk · TLPI Ch14  
- `man 2 mount` · `man 2 umount` · `man 2 umount2` · `man 3 statvfs` · `man 2 statfs` · `man 5 proc`（`mounts`）


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
