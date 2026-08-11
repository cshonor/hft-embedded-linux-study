# TLPI 第 18 章 — Directories and Links

> 对应目录：`chapter-18-directories-links/`  
> 书名原文：**Directories and Links**  
> ⚠️ **目录 = `(名字 → inode#)` 列表；** 硬链接共享 inode，软链接是独立 inode 存路径字符串。`rename` **同 FS 内原子**。

**优先级**：🔴（路径树操作、临时文件、TOCTOU、可靠写入）  
**前置**：[Ch17 ACL](../chapter-17-access-control-lists/notes.md) · [Ch14 FS/inode](../chapter-14-file-systems/notes.md) · [Ch15 stat/lstat](../chapter-15-file-attributes/notes.md)  
**后置**：[Ch19 inotify](../chapter-19-monitoring-file-events/notes.md)

---

## 章节目标

理清目录 / 硬链 / 软链；掌握 mkdir/rmdir、目录遍历、link/symlink/unlink、`rename` 原子性；用 `*at` + flags 缓解路径竞态。

---

## 18.1 目录基础

目录是特殊文件：数据块存目录项 `(文件名, inode#)`。  
`.` = 自己；`..` = 父；根的 `..` 指向自己。

### 目录权限语义

| 位 | 含义 |
|----|------|
| `r` | 可列目录项（见名字）；**不能**单靠它访问内容 |
| `x` | 可进入 / 解析路径内名字（**访问内部文件必备**） |
| `w` | 可在目录内创建、删除、重命名条目 |

> 仅 `r` 无 `x`：能 `ls` 见名，常无法 `stat`/`open` 内部文件。

```c
int mkdir(const char *pathname, mode_t mode);      /* 受 umask */
int mkdirat(int dirfd, const char *pathname, mode_t mode);
int rmdir(const char *pathname);                   /* 仅空目录 */
int chdir(const char *path);
int fchdir(int fd);
char *getcwd(char *buf, size_t size);
```

`rmdir` 非空失败；递归删除须自写（`rm -r` 逻辑）。CWD 是**进程属性**。

---

## 18.2 读目录

```c
DIR *opendir(const char *name);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
```

`d_name` 文件名；`d_ino` 等视平台。  
可移植：类型靠再 `lstat`/`stat`；Linux `d_type` 是扩展。

Demo：[`code/list_dir.c`](./code/list_dir.c)

---

## 18.3 硬链接

```c
int link(const char *oldpath, const char *newpath);
int linkat(...);
int unlink(const char *pathname);
int unlinkat(int dirfd, const char *pathname, int flags); /* AT_REMOVEDIR≈rmdir */
```

多个目录项 → **同一 inode**；`st_nlink++`。

| 规则 | |
|------|--|
| 无主次 | 共享权限/大小/时间戳 |
| 同 FS | 不能跨分区 |
| 不能链目录 | 用户空间禁目录硬链（防环；`.`/`..` 内核特例） |
| 目标须已存在 | |

**删除：** `unlink` 减链接计数；`nlink==0` 且无打开 fd → 释放数据。  
技巧：`open` 后立刻 `unlink` → 进程仍可读写，关 fd 后消失（临时文件）。

Demo：[`code/links_demo.c`](./code/links_demo.c) · [`code/unlink_open.c`](./code/unlink_open.c)

---

## 18.4 符号链接

```c
int symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
```

独立 inode，内容 = 目标路径字符串。可跨 FS、可指目录、可悬空。  
`stat` 跟随；`lstat` 看链接本身；`readlink` 只读字符串（不解析）。

### 硬链 vs 软链

| | 硬链接 | 符号链接 |
|--|--------|----------|
| inode | 共享 | 独立 |
| 跨 FS | ❌ | ✅ |
| 指目录 | ❌（用户） | ✅ |
| 目标删后 | 数据仍在 | 悬空 |
| | | `stat` 跟 / `lstat` 看链 |

---

## 18.5 `rename`（同 FS 原子）

```c
int rename(const char *oldpath, const char *newpath);
int renameat(...);
```

| 情况 | 行为（同 FS） |
|------|----------------|
| new 不存在 | 改名 |
| new 是文件 | 原子替换 |
| new 空目录 | 可替换 |
| new 非空目录 | 失败 |
| **跨 FS** | 不能当原子移动（常需复制+删） |

可靠写入：写临时文件 → `rename(tmp, target)`。

Demo：[`code/rename_safe_write.c`](./code/rename_safe_write.c)

---

## 18.6 路径与 TOCTOU

默认多跟随符号链接 → 检查与使用之间可被换链。  
优先 `openat`/`unlinkat`/`linkat` 等，基于目录 fd；常用  
`AT_SYMLINK_NOFOLLOW`、`AT_REMOVEDIR`。

---

## 18.7 易错清单

1. 目录要访问内容：常需 `x`，不只 `r`  
2. 硬链禁目录；软链可以  
3. `unlink` 减计数，不是立刻抹数据  
4. `rename` 原子性限于**同一文件系统**  
5. `readlink` ≠ 解析最终路径  
6. 硬链不新建数据 inode；软链新建 inode  
7. 无「递归 rmdir」syscall；自行遍历 unlink + rmdir  
8. 根 `..` 指自己，遍历防环  

---

## 练习

1. `opendir`/`readdir` + `lstat` 区分类型  
2. `link` vs `symlink`，比 `st_ino`  
3. 临时文件 + `rename` 安全写  
4. open 后 unlink 仍可读  
5. 目录仅 `r` 无 `x` 的行为  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 目录项存名字→inode；权限 r/x/w 语义不同 |
| 2 | 硬链共享 inode，同 FS，不链目录 |
| 3 | 软链独立 inode，可跨 FS / 悬空 |
| 4 | `nlink==0` 且无 fd → 才释放 |
| 5 | `rename` 同 FS 原子；可靠写 = write tmp + rename |
| 6 | 用 `*at` + `AT_SYMLINK_NOFOLLOW` 减 TOCTOU |

---

## 参考

- Kerrisk · TLPI Ch18  
- `man 2 mkdir` · `man 3 opendir` · `man 2 link` · `man 2 symlink` · `man 2 rename` · `man 2 unlink`
