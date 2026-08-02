# TLPI 第 51 章 — Introduction to POSIX IPC

> 对应目录：`chapter-51-posix-ipc-intro/`  
> 书名原文：**Introduction to POSIX IPC**（POSIX.1b 实时扩展）  
> ⚠️ **导论章。** 统一范式：`open` → 操作 → `close` → **`unlink`（删名）**；对象在**全部 close** 后销毁。POSIX mq 的 fd 可 **epoll**；SysV id 不行。Linux 名建议 `/foo`（中间无多余 `/`）。

**优先级**：🟡（POSIX 三件套地图；对标 SysV）  
**前置**：[Ch45–48 SysV](../chapter-45-sysv-ipc-intro/notes.md) · [Ch49–50 mmap/VM](../chapter-49-memory-mappings/notes.md)  
**后置**：[Ch52 mq](../chapter-52-posix-message-queues/notes.md) → [Ch53 sem](../chapter-53-posix-semaphores/notes.md) → [Ch54 shm](../chapter-54-posix-shared-memory/notes.md)

---

## 章节目标

三类 POSIX IPC；文件风 API 与引用计数；vs SysV 总表；选型。

---

## 51.1 三类机制

| | POSIX | SysV 对应 |
|--|-------|-----------|
| 消息队列 | `mq_*` | msgget… |
| 信号量 | 命名 / 匿名 | semget 集 |
| 共享内存 | `shm_open`+`mmap` | shmget… |

目标：可 epoll、统一回收、类文件 API。

---

## 51.2 统一模型

1. **open**：`mq_open` / `sem_open` / `shm_open`（`O_CREAT|O_EXCL` + mode）→ **fd/句柄**  
2. **操作**：send/recv · wait/post · mmap  
3. **close**：关本进程引用，**不销毁对象**  
4. **unlink**：删名字；**已打开者仍可用**；**全部 close 后内核销毁**

命名（Linux）：`/myqueue` ✅；`/ipc/queue` ❌。挂载点示意：`/dev/mqueue`、`/dev/shm`。

Demo：[`code/`](./code/)（`shm_open` 开闭 unlink）

---

## 51.3 POSIX vs System V（核心表）

| 维度 | System V | POSIX |
|------|----------|-------|
| 名字 | `key_t` / ftok | 字符串路径风 |
| 句柄 | 整型 id，**非 fd** | **fd**（mq 可 epoll） |
| 多路复用 | ❌ | ✅ mq |
| 删除 | RMID 行为不一 | unlink + 引用计数 |
| API | ctl 三套怪异 | open/close/unlink |
| 信号量 | 集、多计数器 | 单个；命名/匿名 |
| 消息 | mtype 筛选 | **优先级** |
| 运维 | ipcs/ipcrm | 常可对 `/dev/*` ls/rm |

SysV 痛点：非 fd、sem 初始化竞态、僵尸对象、`union semun`。  
POSIX 短板：可选组件、部分老系统不全；sem 不能一次原子多计数器。

---

## 51.4 三件预览

- **mq**：优先级；通知/epoll；创建时定 maxmsg/msgsize  
- **sem**：命名（无关进程）/ 匿名（共享内存里，线程或亲缘）  
- **shm**：`shm_open` → `ftruncate` → `mmap`（常在 tmpfs `/dev/shm`）

---

## 选型（TLPI 倾向）

1. 新 Linux → **优先 POSIX**；事件驱动 → POSIX mq  
2. 老 UNIX / 遗留 → SysV  
3. 大批量 → shm + POSIX sem  
4. 简单流 → pipe / UNIX 域 socket  

---

## 思考题要点

1. unlink 删名；close 降引用；全 close 销毁。  
2. mq 返回 fd；SysV 非 fd。  
3. 命名：无关进程；匿名：线程/共享区。  
4. SysV 集可原子多 op；POSIX 单计数器更简单。  
5. unlink 后已打开句柄仍有效。  
6. shm_open → ftruncate → mmap。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | open / close / unlink 文件风 |
| 2 | unlink≠立刻毁；等最后 close |
| 3 | mq fd 可 epoll；SysV 不能 |
| 4 | 名 `/foo`；Linux 在 /dev/mqueue·shm |
| 5 | 新项目优先 POSIX |
| 6 | 细节见 Ch52–54 |

---

## 参考

- Kerrisk · TLPI Ch51（非「第 31 章」误标）  
- `man 7 mq_overview` · `sem_overview` · `shm_overview`
