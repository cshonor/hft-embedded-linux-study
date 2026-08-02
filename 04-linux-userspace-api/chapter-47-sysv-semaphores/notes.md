# TLPI 第 47 章 — System V Semaphores

> 对应目录：`chapter-47-sysv-semaphores/`  
> 书名原文：**System V Semaphores**  
> ⚠️ **`semget` 不初始化计数** → 须 `CREAT|EXCL` 创建者 `SETVAL`，否则竞态。`semop` 多操作 **全有或全无**。`SEM_UNDO` 是崩溃兜底，**无所有权**（谁都能 V）。新项目优先 POSIX 信号量。

**优先级**：🔴（同步原语；配共享内存）  
**前置**：[Ch45 导论](../chapter-45-sysv-ipc-intro/notes.md) · [Ch46 消息队列](../chapter-46-sysv-message-queues/notes.md)  
**后置**：[Ch48 SysV 共享内存](../chapter-48-sysv-shared-memory/notes.md)

---

## 章节目标

信号量集；`semget`/`semctl`/`semop`；初始化竞态；`SEM_UNDO`；二元互斥；限额与缺陷。

---

## 47.1 概念

- 对象是**信号量集**（`semid` + 下标 0..nsems-1），非单个变量。  
- 操作：P（减，不足则阻塞）· V（加，可唤醒）· 等值变为 0（`sem_op==0`）。  
- **不传数据**，只同步；典型保护 SysV shm。  
- **内核持久** → `IPC_RMID` / 重启。

---

## 47.2–47.4 API

### `semget(key, nsems, semflg)`

创建时指定 `nsems`；打开已有集时 `nsems` 不能大于集大小。  
⚠️ **只分配，不初始化数值**。

### `semctl` · 须自声明 `union semun`

| cmd | |
|-----|--|
| `SETVAL` / `GETVAL` | 单个 |
| `SETALL` / `GETALL` | 整集 |
| `IPC_STAT` / `IPC_SET` | 属性 |
| `IPC_RMID` | **立即删整集**；唤醒阻塞者 |

### `semop(semid, sops, nsops)`

```c
struct sembuf { unsigned short sem_num; short sem_op; short sem_flg; };
```

| `sem_op` | 行为 |
|----------|------|
| `>0` | V，不阻塞 |
| `<0` | P；值不够则阻塞（或 `IPC_NOWAIT`→`EAGAIN`） |
| `==0` | 等变为 0 |

`sem_flg`：`IPC_NOWAIT` · **`SEM_UNDO`**。  
多条 `sembuf`：**原子 all-or-nothing**（可一次拿多锁，减死锁）。

---

## 47.5 初始化竞态（经典坑）

A `semget(CREAT)` 尚未 `SETVAL`，B 已 `semop` → 值未定义。

**解法**：仅 `IPC_CREAT|IPC_EXCL` 成功者初始化；`EEXIST` 则再 `semget` 打开。

```c
semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600);
if (semid >= 0) {
    union semun su = { .val = 1 };
    semctl(semid, 0, SETVAL, su);
} else if (errno == EEXIST) {
    semid = semget(key, 1, 0600);
}
```

Demo：[`code/`](./code/)

---

## 47.8 `SEM_UNDO`

带此标志的 `semop` 记入进程 undo 表；进程退出（含 kill）内核回放反向操作。  
局限：只跟**本进程**；`IPC_RMID` 后 undo 失效；多次 P 叠加 undo；**不能替代正常 V**。

---

## 47.9 二元信号量 ≈ 互斥

初值 1；P=`-1`；V=`+1`（常加 `SEM_UNDO`）。  
⚠️ **无所有权**：任意进程可 V「别人」持有的锁（异于 `pthread_mutex`）。

---

## 47.10–47.11 限额 · 缺陷

`SEMMNI`/`SEMMNS`/`SEMMSL`/`SEMOPM`/`SEMVMX`(32767) — 见 `/proc/sys/kernel/sem`。  
`ipcs -s` · `ipcrm -s id`。

缺陷：非 fd、创建/初始化两步竞态、内核持久、API 笨重、无超时、集设计过剩。TLPI：优先 **POSIX 信号量**。

---

## 思考题要点

1. 上节安全初始化。  
2. `SEM_UNDO` 兜底 ≠ 防死锁；无超时仍可永久阻塞。  
3. 多 `sembuf` 原子 → 减死锁。  
4. `IPC_RMID` → 阻塞 `semop` 失败返回（`EIDRM`）。  
5. 无所有者 vs mutex。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 信号量**集**；get/ctl/op |
| 2 | get 不初始化 → EXCL 创建者 SETVAL |
| 3 | sem_op：+/−/0；多 op 原子 |
| 4 | SEM_UNDO = 退出撤销 |
| 5 | 无所有权；内核持久 |
| 6 | 新项目用 POSIX sem |

---

## 参考

- Kerrisk · TLPI Ch47（非「第 20 章」误标）  
- `man 2 semget` · `semop` · `semctl`
