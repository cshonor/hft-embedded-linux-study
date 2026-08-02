# TLPI 第 45 章 — Introduction to System V IPC

> 对应目录：`chapter-45-sysv-ipc-intro/`  
> 书名原文：**Introduction to System V IPC**（又称 XSI IPC）  
> ⚠️ **英文原版是 Ch45**；部分中文分册把本章标成「第 10 章」属下册重编号，以 [CHAPTER-MAP](../CHAPTER-MAP.md) 为准。  
> ⚠️ **内核持久：崩溃忘 `IPC_RMID` 会泄漏。** id **不是 fd**，不能 `epoll`。`ftok` 依赖的文件若删重建 → key 变。

**优先级**：🟡（SysV 三机制共用模型）  
**前置**：[Ch44 管道与 FIFO](../chapter-44-pipes-fifos/notes.md)  
**后置**：[Ch46 SysV 消息队列](../chapter-46-sysv-message-queues/notes.md) → [Ch47 信号量](../chapter-47-sysv-semaphores/notes.md) → [Ch48 共享内存](../chapter-48-sysv-shared-memory/notes.md)

---

## 章节目标

三类对象共用的 key/`get`/`ctl`/操作 API；`ipc_perm`；`get` 算法；内核持久与 `ipcs`/`ipcrm`；限额；`IPC_RMID` 差异；与 POSIX 对比铺垫。

---

## 45.1 三类对象 · 统一 API

| 操作 | 消息队列 | 信号量 | 共享内存 | 类比 |
|------|----------|--------|----------|------|
| 创建/打开 | `msgget` | `semget` | `shmget` | open |
| 控制/删 | `msgctl` | `semctl` | `shmctl` | fcntl |
| 主体操作 | `msgsnd`/`msgrcv` | `semop` | `shmat`/`shmdt` | 读写 |

---

## 45.2 Key（`key_t`）

跨进程定位同一对象 ≈「文件名」。

| 方式 | 说明 |
|------|------|
| `IPC_PRIVATE` | 每次 `get` 新建；适合亲缘进程（传 id） |
| 硬编码整数 | 易冲突 |
| **`ftok(path, proj_id)`** | inode+设备+`proj_id`；文件重建 → key 变 |

---

## 45.3–45.4 标识符 · `ipc_perm`

- **key**：外部约定名  
- **id**（msgid/semid/shmid）：内核句柄；后续全用 id  
- **不是 fd** → 不能 select/poll/epoll  

`struct ipc_perm`：uid/gid、cuid/cgid、mode（r/w，无执行语义）。权限逻辑类似文件。

---

## 45.5 `get()` 算法（高频）

`msgget(key, flag)`，`flag` = `IPC_CREAT` / `IPC_EXCL` + mode：

1. 按 key 查找  
2. **不存在**：无 `IPC_CREAT` → `ENOENT`；有则创建  
3. **已存在**：`CREAT|EXCL` → `EEXIST`；仅 `CREAT` → 返回已有 id  

服务端常用：`get(key, IPC_CREAT | IPC_EXCL | 0600)`。

Demo：[`code/`](./code/)

---

## 45.6 内核持久 · 运维

进程全退对象仍在 → `*ctl(..., IPC_RMID)` 或重启。

```bash
ipcs -q/-s/-m
ipcrm -q <msqid>    # 或 -M key / -S / -m 等
```

---

## 45.7–45.8 缺陷与限额

1. 非 fd，无多路复用  
2. 易泄漏，无自动回收  
3. 无路径名，`ls`/`rm` 管不了  
4. API 老旧；新项目优先 POSIX IPC  

限额：`/proc/sys/kernel/msg*`、`sem`、`shm*`；超限常 `ENOSPC`。

---

## 45.9 `IPC_RMID` 差异

| 对象 | 行为 |
|------|------|
| 消息队列 / 信号量集 | 立即标记删除；持有 id 者可继续用，引用尽后释放 |
| 共享内存 | 仅标记；**全部 `shmdt` 后才真正销毁** |

---

## 思考题要点

1. `IPC_PRIVATE`：亲缘；`ftok`：无关进程约定路径。  
2. `ftok`：固定路径文件勿删；或固定 key + 约定文档。  
3. key≠id；id 非 fd → 无 epoll。  
4. `CREAT|EXCL`：独占创建。  
5. 内核持久 + 忘 RMID → 泄漏占限额。  
6. shm 的 RMID 须等 detach。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | get / ctl / 操作 三套对称 API |
| 2 | key 定位；id 操作；**非 fd** |
| 3 | `CREAT\|EXCL` 独占创建 |
| 4 | 内核持久 → `IPC_RMID` / `ipcrm` |
| 5 | `ftok` 吃 inode；文件重建 key 变 |
| 6 | shm 的 RMID 等全部 shmdt |

---

## 参考

- Kerrisk · TLPI **Ch45**（非中文分册「第 10 章」）  
- `man 3 ftok` · `man 1 ipcs` · `man 1 ipcrm`
