# TLPI 第 15 章 — File Attributes

> 对应目录：`chapter-15-file-attributes/`  
> 书名原文：**File Attributes**  
> ⚠️ **属性在 inode：** `stat`/`lstat`/`fstat` 读元数据；`ctime` ≠ 创建时间，且**不能**由程序直接设定。

**优先级**：🔴（权限 / 安全 / 目录与链接编程基础）  
**前置**：[Ch14 File Systems](../chapter-14-file-systems/notes.md)（inode 模型）  
**后置**：[Ch16 Extended Attributes](../chapter-16-extended-attributes/notes.md) · [Ch18 目录与链接](../chapter-18-directories-links/notes.md) · [Ch38 特权与安全](../chapter-38-secure-privileged/notes.md)

---

## 章节目标

掌握 `stat` 系列读 inode；解析类型与权限；会改权限/属主/时间戳；理解 SUID/SGID/粘滞位与 atime/mtime/ctime；理清接口权限约束；警惕 `access()`。

---

## 15.1 `stat` / `lstat` / `fstat`

```c
#include <sys/stat.h>
int  stat(const char *pathname, struct stat *buf);   /* 跟随符号链接 */
int lstat(const char *pathname, struct stat *buf);   /* 链接自身 */
int fstat(int fd, struct stat *buf);                 /* 已打开 fd；减 TOCTOU */
```

| 调用 | 符号链接 |
|------|----------|
| `stat` | 跟到目标 |
| `lstat` | 得到链接 inode（`S_ISLNK`） |
| `fstat` | 看 fd 所指对象（打开时已解析） |

### `struct stat` 核心字段

| 字段 | 含义 |
|------|------|
| `st_dev` / `st_ino` | 设备 + inode（同 FS 内唯一） |
| `st_mode` | 类型 + 权限 + 特殊位 |
| `st_nlink` | 硬链接计数 |
| `st_uid` / `st_gid` | 属主 / 属组 |
| `st_rdev` | 设备文件：主/次设备号 |
| `st_size` | 字节大小 |
| `st_blksize` / `st_blocks` | 优选 IO 块；占用块数（常 512B 单位） |
| `st_atim` / `st_mtim` / `st_ctim` | 访问 / 内容修改 / inode 变更（现代为 `timespec`） |

Demo：[`code/print_stat.c`](./code/print_stat.c)

---

## 15.2 `st_mode`：类型与权限

### 类型宏

`S_ISREG` · `S_ISDIR` · `S_ISLNK` · `S_ISCHR` · `S_ISBLK` · `S_ISFIFO` · `S_ISSOCK`

### 九位权限

`S_IRUSR/WUSR/XUSR` · `S_IRGRP/...` · `S_IROTH/...`

### 特殊位

| 位 | 含义 |
|----|------|
| `S_ISUID` | Set-User-ID |
| `S_ISGID` | Set-Group-ID |
| `S_ISVTX` | 粘滞位：在**目录**上 → 仅属主/root 可删目录内条目（如 `/tmp`） |

> 普通文件上的 sticky：现代 Linux 基本无实际意义。

---

## 15.3 `chmod` / `fchmod`

```c
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
```

- 属主或 root 可改  
- `mode` 是**最终**权限，非增量  
- 普通用户设 SUID/SGID 有安全限制  
- 创建时 `open`/`mkdir` 的 mode 还受 **`umask`** 裁剪  

---

## 15.4 `chown` / `fchown` / `lchown`

```c
int  chown(const char *path, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group);  /* 不跟随链接 */
int fchown(int fd, uid_t owner, gid_t group);
```

Linux 常见约束：

| 谁 | 能做什么 |
|----|----------|
| root | 任意改 uid/gid |
| 普通用户 | 不能把属主改成别人；属组通常只能改成自己所属组 |

`owner`/`group` 传 `(uid_t)-1` / `(gid_t)-1`：**不改该项**。

---

## 15.5 atime / mtime / ctime

| 时间 | 何时更新 |
|------|----------|
| **atime** | 读数据（`read` 等）；挂载 `noatime` 可关，提性能 |
| **mtime** | 内容变（`write`/`truncate`） |
| **ctime** | inode 元数据变（`chmod`/`chown`/`link`/`unlink`/`rename` 等） |

⚠️ **ctime 不是创建时间**；程序**不能**直接设定 ctime。  
经典 ext 不强调 birth time；较新内核可用 **`statx`** 取 **btime**。

### 改 atime/mtime

```c
int utime(const char *path, const struct utimbuf *times);      /* 旧 */
int utimes(const char *path, const struct timeval times[2]);
int futimens(int fd, const struct timespec times[2]);          /* 推荐，纳秒 */
int utimensat(int dirfd, const char *pathname,
              const struct timespec times[2], int flags);
```

- `times[0]`=atime，`times[1]`=mtime  
- `times == NULL` → 都设为现在  
- `UTIME_NOW` / `UTIME_OMIT` 可选改一项  
- `AT_SYMLINK_NOFOLLOW`：不跟随链接  

Demo：[`code/futimens_demo.c`](./code/futimens_demo.c)

---

## 15.6 `umask`

```c
mode_t umask(mode_t mask);   /* 设新值，返回旧值 */
```

最终权限 ≈ `创建 mode & ~umask`  
例：`0666` + umask `022` → `0644`  
只影响**新建**，不改已有文件。

Demo：[`code/umask_demo.c`](./code/umask_demo.c)

---

## 15.7 `access()` — 尽量别用

```c
int access(const char *pathname, int mode);  /* R_OK W_OK X_OK F_OK */
```

| 问题 | |
|------|--|
| 用 **真实** UID/GID 测 | 实际打开用 **有效** UID/GID → setuid 下易误判 |
| TOCTOU | 测完再 `open` 之间状态可变 |

安全做法：直接 `open`，处理 `EACCES`。

---

## 15.8 速查：`stat` 族 vs `statx` · 时间接口

| API | 可移植 | 链接 | 备注 |
|-----|--------|------|------|
| `stat` | ✅ | 跟随 | 经典 |
| `lstat` | ✅ | 不跟随 | 看链接本身 |
| `fstat` | ✅ | — | 推荐：已有 fd |
| `statx` | Linux 4.11+ | `AT_SYMLINK_NOFOLLOW` 等 | 纳秒、btime、按字段 mask |

| 时间接口 | 精度 | 备注 |
|----------|------|------|
| `utime` | 秒 | 旧 |
| `utimes` | 微秒 | |
| `futimens` / `utimensat` | 纳秒 | **SUSv4 推荐** |

---

## 15.9 `statx`（Linux）

```c
int statx(int dirfd, const char *pathname, int flags,
          unsigned int mask, struct statx *buf);
```

优势：纳秒、btime、统一 flags、按需字段。  
可移植代码仍用 `stat`/`lstat`/`fstat`。

Demo（可选）：[`code/statx_btime.c`](./code/statx_btime.c)

---

## 15.10 易错清单

1. `stat` 跟随；`lstat` 才见 `S_ISLNK`  
2. ctime ≠ 创建时间；改元数据会动 ctime  
3. 不能手动设 ctime  
4. sticky 对**目录**有意义  
5. 改时间用 `futimens`/`utimensat`，不是 `chmod`  
6. 避开 `access()`（RUID vs EUID + TOCTOU）  
7. `st_ino` 仅同 FS 可比  
8. 创建 mode 被 umask 裁剪  

---

## 练习

1. 简易 `ls`：`lstat` 打类型/权限/属主/大小/时间  
2. 不同 umask 下建文件比权限  
3. `futimens` 改 atime/mtime，看 ctime 变  
4. （选）setuid 场景下 `access` 误导  
5. （选）`statx` 取 btime  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `lstat` 看链接本身；`fstat` 减路径竞争 |
| 2 | atime 读 / mtime 内容 / ctime inode；ctime 不可设 |
| 3 | 最终权限 ≈ mode & ~umask |
| 4 | sticky 目录：防他人删你的文件（`/tmp`） |
| 5 | 别用 `access` 做安全决策 |
| 6 | Linux 要 birth time → `statx` |

---

## 参考

- Kerrisk · TLPI Ch15  
- `man 2 stat` · `man 2 chmod` · `man 2 chown` · `man 2 utimensat` · `man 2 umask` · `man 2 access` · `man 2 statx`
