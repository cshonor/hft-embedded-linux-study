# TLPI 第 53 章 — POSIX Semaphores

> 对应目录：`chapter-53-posix-semaphores/`  
> 书名原文：**POSIX Semaphores**  
> ⚠️ **单个计数器，无信号量集。** 命名：`sem_open`；匿名：`sem_init`（跨进程须 `pshared≠0` 且放**共享内存**）。**无 SEM_UNDO** → 持锁崩溃易死锁。无所有权（异于 `pthread_mutex`）。链接 `-pthread`（偶需 `-lrt`）。

**优先级**：🔴（进程/线程同步）  
**前置**：[Ch52 POSIX mq](../chapter-52-posix-message-queues/notes.md) · [Ch47 SysV sem](../chapter-47-sysv-semaphores/notes.md)  
**后置**：[Ch54 POSIX 共享内存](../chapter-54-posix-shared-memory/notes.md)

---

## 章节目标

命名 / 匿名；wait/post/timed；fork/exec；vs SysV / mutex。

---

## 53.1 概念

| | 命名 | 匿名 |
|--|------|------|
| 定位 | `/name`，无关进程 | 内存中的 `sem_t` |
| 场景 | 任意本机进程 | 线程；或 `pshared`+共享内存的亲缘/共享区进程 |
| 生命周期 | unlink + 全 close | 随内存；`sem_destroy` |

`sem_wait` P / `sem_post` V；值非负；**无所有权**。

---

## 53.2 命名

```c
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned value);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);
```

`O_CREAT|O_EXCL`：创建+初值原子，**无 SysV 式初始化竞态**。  
unlink 删名；全 close 销毁。

---

## 53.3 操作（两形态共用）

`sem_wait` · `sem_trywait` · `sem_timedwait`（**绝对** `CLOCK_REALTIME`）· `sem_post` · `sem_getvalue`（瞬时快照，勿当同步条件）。

---

## 53.4 匿名

```c
int sem_init(sem_t *sem, int pshared, unsigned value);
int sem_destroy(sem_t *sem);
```

| `pshared` | |
|-----------|--|
| `0` | 线程间；普通进程内存 |
| `≠0` | 进程间；`sem_t` **必须在共享内存** |

勿对命名调 `sem_destroy`；勿对匿名调 `sem_open/close`。

Demo：[`code/`](./code/)

---

## 53.5 fork / exec

| | 命名 | 匿名 pshared=0 | 匿名 pshared≠0 |
|--|------|----------------|----------------|
| fork | 同内核对象 | **各自副本，不同步** | 共享内存内同一 sem |
| exec | 自动 close | 映射毁 → 失效 | 同左 |

---

## 53.6 vs SysV · vs mutex

| | SysV | POSIX |
|--|------|-------|
| 形态 | 集、多计数器 | 单个 |
| 初始化 | get+ctl 竞态 | open/init 原子 |
| 多 op 原子 | `semop` ✅ | ❌ |
| 崩溃撤销 | `SEM_UNDO` | **无** |
| 超时 | 弱 | `sem_timedwait` |

工程：多锁原子 / SEM_UNDO → SysV；简洁单计数器 → POSIX。  
线程互斥优先 **`pthread_mutex`**（有所有权、常更快）；跨进程用信号量。

---

## 陷阱

1. 跨进程匿名：`pshared`+共享内存  
2. API 混用 destroy/close  
3. 无 SEM_UNDO  
4. timed 绝对时间  
5. getvalue TOCTOU  
6. pshared=0 fork 后不可跨进程  
7. 名 `/a/b` 非法  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 命名 vs 匿名；单个计数器 |
| 2 | open 原子初值；无 init 竞态 |
| 3 | 跨进程匿名 → 共享内存 |
| 4 | 无 SEM_UNDO；无所有权 |
| 5 | 线程互斥用 mutex |
| 6 | unlink / 全 close 销毁命名对象 |

---

## 参考

- Kerrisk · TLPI Ch53  
- `man 3 sem_overview` · `sem_open` · `sem_init`
