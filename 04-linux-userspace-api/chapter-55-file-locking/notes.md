# TLPI 第 55 章 — File Locking

> 对应目录：`chapter-55-file-locking/`  
> 书名原文：**File Locking**  
> ⚠️ **默认劝告锁：不调 API 的进程可读可写。** `fcntl` 绑 **进程+inode**：关**任意**指向该文件的 fd → **该进程全部记录锁释放**。`flock` 绑 **OFD**：dup 共享锁，任一 close 可放锁。Linux 上 **flock 与 fcntl 互不感知 — 勿混用**。

**优先级**：🔴（文件同步；单实例 pid 文件）  
**前置**：[Ch54 POSIX shm](../chapter-54-posix-shared-memory/notes.md) · 文件 I/O  
**后置**：[Ch56 Sockets 导论](../chapter-56-sockets-intro/notes.md)

---

## 章节目标

`flock` / `fcntl` 记录锁；绑定对象与 close/fork；劝告 vs 强制；pid 文件；死锁与对比。

---

## 55.1 模型

多读单写：共享读锁可并存；排他写锁独占区域。  
默认 **Advisory**；强制锁 Linux 特有且不推荐。

---

## 55.2 `flock`

```c
int flock(int fd, int operation);  /* LOCK_SH|EX|UN [| LOCK_NB] */
```

| 规则 | |
|------|--|
| 粒度 | **整文件** |
| 绑定 | **打开文件描述（OFD）** |
| dup/fork 出的 fd | 同 OFD → **共享同一 flock**；任一 close 可放锁 |
| 同文件不同 open | 不同 OFD → 独立锁 |
| fork | **不继承** flock |
| vs fcntl | Linux **互不阻塞** |

---

## 55.3–55.4 `fcntl` 记录锁

```c
struct flock { short l_type, l_whence; off_t l_start, l_len; pid_t l_pid; };
/* F_SETLK / F_SETLKW / F_GETLK */
```

- `l_len=0`：从 start **锁到 EOF（含后续追加）**  
- `F_GETLK`：仅探测（TOCTOU）；冲突填 `l_pid`  
- 读锁需可读打开；写锁需可写打开  
- 绑定 **进程 + inode**；同进程多 fd 新锁覆盖同区旧锁  
- **任一 fd close → 该进程对此文件全部记录锁没了**  
- fork：**不继承**；退出自动释放；死锁阻塞可 `EDEADLK`

`lockf`：fcntl 封装，仅排他锁，与 fcntl **互通**。

Demo：[`code/`](./code/)

---

## 55.5 劝告 vs 强制（Linux）

强制：挂载 `-o mand` + 文件 `g+s,g-x`。IO 被内核拦截。  
TLPI：慢、有坑、不可移植 → **别用**。

---

## 55.6–55.9 对比 · 运维 · 场景

| | flock | fcntl |
|--|-------|-------|
| 标准 | BSD | POSIX |
| 区域 | 整文件 | 任意字节 |
| 绑定 | OFD | 进程+inode |
| close | 该 OFD 的锁 | **进程全锁** |
| 强制锁 | 否 | 可（勿用） |

`/proc/locks` 查看。  
**PID 文件单实例**：`F_WRLCK` 成功 → 写 pid；失败 → 已有实例；崩溃内核放锁。

---

## 陷阱

1. fcntl：关「无关」fd 也放全锁  
2. flock：dup 后 close 放锁  
3. flock+fcntl 混用失效  
4. GETLK 非原子获取  
5. stdio：锁内外 `fflush`；关键路径用 `read`/`write`  
6. NFS 锁不可靠  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 默认劝告；协作才有效 |
| 2 | flock=OFD 整文件；fcntl=进程+区 |
| 3 | fcntl：任意 close → 全锁放 |
| 4 | fork 两类都不继承锁 |
| 5 | 勿混用 flock/fcntl |
| 6 | pid 文件用 F_WRLCK 单实例 |

---

## 参考

- Kerrisk · TLPI Ch55（非「第 14 章」误标）  
- `man 2 flock` · `fcntl` · `man 3 lockf`
