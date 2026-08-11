# TLPI 第 14 章 — File Systems

> 对应目录：`chapter-14-file-systems/`  
> 书名原文：**File Systems**  
> ⚠️ **链路：** 块/字符设备 → 分区上的 FS（inode/超级块）→ **VFS** → 应用 `open`/`read`/`write`。文件名在**目录数据块**，不在 inode。

**优先级**：🔴（路径解析、挂载、硬链接、持久化与 fsck/日志理解）  
**前置**：[Ch13 File I/O Buffering](../chapter-13-file-io-buffering/notes.md)  
**后置**：[Ch15 File Attributes](../chapter-15-file-attributes/notes.md) · [Ch18 目录与链接](../chapter-18-directories-links/notes.md) · [Ch19 inotify](../chapter-19-monitoring-file-events/notes.md)

---

## 章节目标

理解分区与传统 FS 布局（以 ext2 为范例）、**inode** 与路径解析；掌握 **VFS** 抽象；区分磁盘 FS / tmpfs / 日志 FS；会 `mount`/`umount` 与 `statvfs`；打通硬件 ↔ VFS ↔ 应用。

---

## 14.1 设备特殊文件

`/dev` 下节点代表设备；内容不在该「文件」里，内核用**主/次设备号**找驱动。

| 类型 | 访问方式 | 例子 |
|------|----------|------|
| **块设备** | 可随机、按块、可经页缓存 | `/dev/sda1`、NVMe 分区 |
| **字符设备** | 流式、通常无块缓冲语义 | `/dev/tty`、串口 |

---

## 14.2 磁盘与分区

一块盘可多分区；**一分区上 `mkfs` 出一个独立文件系统**。  
Linux **单一目录树**：各 FS 通过**挂载**接到树上，应用只见路径。

---

## 14.3 传统布局（ext2 范例）

| 区域 | 作用 |
|------|------|
| **Boot block** | 引导；首块常保留 |
| **Superblock** | 全局元数据：块数、inode 数、块大小、空闲量、类型/挂载状态；ext2 多处备份 |
| **inode table** | 所有 inode |
| **Data blocks** | 文件内容、**目录条目** |

> **文件名不在 inode 里**，在目录的数据块：`(name → inode#)`。

---

## 14.4 i-node（核心）

同一 **文件系统内** inode 号唯一；跨 FS 比较 inode **无意义**。硬链接**不能跨分区**。

inode 常含：类型与权限、UID/GID、硬链接计数、大小、atime/mtime/ctime、数据块指针（直接/一二级间接等）。

| 是 | 否 |
|----|-----|
| 元数据 + 块指针 | **文件名** |

目录 = 特殊文件，数据块存名字映射。

### 路径解析 `/home/xxx/file.txt`（示意）

根 inode → 找 `home` → `xxx` → `file.txt` 的 inode → 按块指针读数据。

Demo：[`code/print_inode.c`](./code/print_inode.c)

---

## 14.5 VFS（Virtual File System）

统一 `open`/`read`/`write`；下层可以是 ext4 / xfs / tmpfs / nfs…

| 对象 | 角色 |
|------|------|
| `super_block` | 一次挂载实例 |
| `inode` | FS 内对象 |
| `dentry` | 名字 ↔ inode 缓存（**内存**，加速查找） |
| `file` | 打开文件上下文（与 fd 对应） |

---

## 14.6 日志 FS 与内存 FS

| | |
|--|--|
| **ext2（无日志）** | 崩溃后常需全盘 `fsck`，大盘极慢 |
| **日志 FS** | 先记事务再改正式结构；崩溃后重放日志 |

日志模式（概念）：

| 模式 | 要点 |
|------|------|
| Journal | 元数据+数据都进日志（最稳、最慢） |
| **Ordered**（ext4 常见默认） | 先写数据，再记元数据日志 |
| Writeback | 主要保证元数据；数据顺序弱、更快、风险大 |

常见：ext3/4、XFS、Btrfs…  
**tmpfs**：全在内存（+可 swap）；断电丢；`/dev/shm` 常为 tmpfs。

> 日志**默认不替代**用户数据 `fsync`；元数据恢复 ≠ 「你的 write 已落盘」。见 [Ch13](../chapter-13-file-io-buffering/notes.md)。

---

## 14.7 挂载点与单一目录树

分区 FS 必须挂到某目录（挂载点）才可访问。  
挂载后：**挂载点原内容被遮住**；卸载后恢复。

- 同一 FS 可挂到多处  
- 可在已有树上再叠一层挂载  

---

## 14.8 `mount` / `umount`（需特权，如 `CAP_SYS_ADMIN`）

```c
#include <sys/mount.h>
int mount(const char *source, const char *target,
          const char *fstype, unsigned long mountflags,
          const void *data);
int umount(const char *target);
int umount2(const char *target, int flags);
```

### 常用 `mountflags`

| 标志 | 含义 |
|------|------|
| `MS_RDONLY` | 只读 |
| `MS_NOEXEC` | 禁止执行 |
| `MS_NOSUID` | 忽略 setuid/setgid |
| `MS_REMOUNT` | 改挂载选项（不卸） |
| `MS_BIND` | 绑定挂载（目录树镜像） |

### `umount2`

| 标志 | 含义 |
|------|------|
| `MNT_FORCE` | 强制；忙时慎用，易丢数据 |
| `MNT_DETACH` | 懒卸载：已打开可继续，引用尽再真正卸 |

默认：仍有打开文件、或 cwd 在该 FS 上 → `umount` 失败（`EBUSY`）。

Demo（需 root）：[`code/mount_bind_demo.c`](./code/mount_bind_demo.c)

---

## 14.9 文件系统信息 API

```c
#include <sys/statfs.h>   /* Linux */
int statfs(const char *path, struct statfs *buf);
int fstatfs(int fd, struct statfs *buf);

#include <sys/statvfs.h>  /* SUSv3，可移植优先 */
int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);
```

字段侧重：块大小、总/可用块、inode 总量/空闲。`df` 一类工具底层用这套。  
⚠️ 结构体不同；**可移植代码用 `statvfs`**。

内核挂载表：读 `/proc/mounts`（或 `/proc/self/mounts`）。

Demo：[`code/mini_df.c`](./code/mini_df.c) · [`code/proc_mounts.c`](./code/proc_mounts.c)

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

## 14.11 易错清单

1. inode 号仅在**同一 FS** 内可比；硬链接不能跨 FS  
2. `unlink`：链接计数→0 且无打开者 → 才真正释放数据块  
3. 挂载遮盖：别把重要目录当挂载点却忘了「底下有东西」  
4. tmpfs 吃内存/swap，不占普通磁盘配额语义  
5. dentry 是**内核缓存**，不落盘  
6. 日志 ≠ 用户数据已持久化；关键写仍要 `fsync`/`fdatasync`  
7. 可移植：`statvfs`；挂载信息可解析 `/proc/mounts`  

---

## 练习

1. `statvfs` 简易 `df`  
2. root 下 `MS_BIND` 绑定挂载再卸载  
3. 同分区硬链接 inode 相同；跨分区对比 inode  
4. （选）`MS_RDONLY` / `MS_NOEXEC` 行为  
5. 解析 `/proc/mounts`  

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

## 参考

- Kerrisk · TLPI Ch14  
- `man 2 mount` · `man 2 umount` · `man 2 umount2` · `man 3 statvfs` · `man 2 statfs` · `man 5 proc`（`mounts`）
