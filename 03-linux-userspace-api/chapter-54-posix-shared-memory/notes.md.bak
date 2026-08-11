# TLPI 第 54 章 — POSIX Shared Memory

> 对应目录：`chapter-54-posix-shared-memory/`  
> 书名原文：**POSIX Shared Memory**  
> ⚠️ **流程：`shm_open` → `ftruncate` → `mmap(MAP_SHARED)`。** 新建 size=0，忘 `ftruncate` → 访问 **SIGBUS**。须另配信号量。段内只用 **offset**。后置按地图是 [Ch55 文件锁](../chapter-55-file-locking/notes.md)（非直接跳 socket）。

**优先级**：🔴（POSIX IPC 终章；无关进程高速区）  
**前置**：[Ch53 POSIX sem](../chapter-53-posix-semaphores/notes.md) · [Ch48 SysV shm](../chapter-48-sysv-shared-memory/notes.md) · [Ch49 mmap](../chapter-49-memory-mappings/notes.md)  
**后置**：[Ch55 File Locking](../chapter-55-file-locking/notes.md)

---

## 章节目标

`shm_open`/`ftruncate`/`mmap`/`unlink`；生命周期；vs SysV shm / 匿名共享 mmap；工程模板。

---

## 54.1 原理

本质：`/dev/shm` **tmpfs** 上的命名对象（常驻 RAM，默认不落盘）。  
**无内置同步** → 配 POSIX 信号量等。  

| | POSIX 命名 shm | `MAP_SHARED\|ANONYMOUS` |
|--|----------------|-------------------------|
| 进程 | 无关进程（同名） | **仅亲缘**（fork） |

---

## 54.2–54.4 标准流程

```c
int fd = shm_open("/shmbuf", O_CREAT|O_RDWR, 0600);
ftruncate(fd, len);   /* 新建为 0，必须扩展 */
void *p = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
```

- 名：`/foo`；无多层 `/`  
- 返回**普通 fd**（`fstat`/`fcntl` 可用）  
- mmap **必须 `MAP_SHARED`**（`PRIVATE` 改不可见）  
- 缩小后越过长度 → SIGBUS  

Demo：[`code/`](./code/)

---

## 54.5 生命周期

`close(fd)`：降引用，不销毁。  
`shm_unlink(name)`：删名。  
真正销毁：已 unlink + **全部 close + 全部 munmap**。  

`fork`：继承 fd 与映射。  
`exec`：映射销毁；fd 默认保留（可 `FD_CLOEXEC`）。

---

## 54.6 vs System V shm

| | SysV | POSIX |
|--|------|-------|
| 句柄 | shmid 非 fd | **fd** |
| 后端 | 内核对象 | `/dev/shm` |
| 大小 | 创建固定 | `ftruncate` 可调 |
| 映射 | shmat/dt | mmap/munmap |
| 运维 | ipcs | `ls /dev/shm` |
| 重启 | 内核持久至 RMID | tmpfs 清 |

工程：**POSIX shm + 命名/共享区信号量**；通信完 munmap/close；一方 unlink。

---

## IPC 路线收束（Ch43–54）

Pipe/FIFO → SysV 三件套 → mmap/VM → POSIX 三件套。  
下一章地图：**Ch55 文件锁** → 再进 Socket（Ch56+）。

---

## 陷阱

1. 忘 ftruncate → SIGBUS  
2. `MAP_PRIVATE` 不共享  
3. 无同步竞态  
4. 名格式错误  
5. 混淆匿名共享 mmap  
6. 忘 unlink → `/dev/shm` 残留  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | open → ftruncate → mmap SHARED |
| 2 | 新建 size=0；SIGBUS 常见因未扩 |
| 3 | unlink + 全 close/munmap 销毁 |
| 4 | 须同步；段内用 offset |
| 5 | 无关进程用 shm_open，非匿名 SHARED |
| 6 | 新项目常优于 SysV shm |

---

## 参考

- Kerrisk · TLPI Ch54  
- `man 3 shm_open` · `man 7 shm_overview`
