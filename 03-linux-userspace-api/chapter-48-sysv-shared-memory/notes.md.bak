# TLPI 第 48 章 — System V Shared Memory

> 对应目录：`chapter-48-sysv-shared-memory/`  
> 书名原文：**System V Shared Memory**  
> ⚠️ **最快本机 IPC，但无内置同步。** `IPC_RMID` 只打删除标记，**`shm_nattch==0` 才真正释放**。段内禁存裸指针，只存 **offset**。新项目可优先 POSIX shm / `mmap`。

**优先级**：🔴（无拷贝；配信号量）  
**前置**：[Ch47 SysV 信号量](../chapter-47-sysv-semaphores/notes.md)  
**后置**：[Ch49 mmap](../chapter-49-memory-mappings/notes.md) · [Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/notes.md)

---

## 章节目标

`shmget`/`shmat`/`shmdt`/`shmctl`；延迟 `IPC_RMID`；指针陷阱；shm+sem 模型；限额与对比。

---

## 48.1 原理

多进程映射同一物理页 → **无用户↔内核数据拷贝**。  
本身不同步 → 须信号量/文件锁。  
**内核持久**；detach ≠ 销毁。  
术语：SysV 说 **attach/detach**；`mmap` 说 map — 思想近、API 不同。

---

## 48.2–48.3 · 48.7 API

### `shmget(key, size, shmflg)`

`size` 向上对齐页；新建清零。返回 `shmid`。

### `shmat(shmid, shmaddr, shmflg)`

- `shmaddr=NULL`：**推荐**（内核选址）  
- `SHM_RDONLY`：只读  
- 成功：起始虚址；失败：`(void*)-1`  
- `fork` 继承映射；**`exec` 全部自动 detach**

### `shmdt(addr)`

解除本进程映射；**不销毁**内核段。进程退出时内核自动 detach。

### `shmctl` · `IPC_RMID`（高频）

| 对象 | `IPC_RMID` |
|------|------------|
| mq / sem | 立即标记删；新 get 失败 |
| **shm** | 仅 `SHM_DEST`；**全部 shmdt（nattch=0）后才释放** |

`IPC_STAT`/`IPC_SET`：属性；关注 `shm_nattch`、`shm_segsz`。

Demo：[`code/`](./code/)

---

## 48.6 陷阱：段内勿存指针

各进程映射**虚址可能不同** → 绝对指针在对方无效 → SIGSEGV。  
✅ 只存相对段基址的 **offset**（或固定布局 struct，无堆指针）。

---

## 48.4 工程模型

**SysV sem（互斥/握手）+ SysV shm（大数据）** → 一方 `IPC_RMID` 打标 → 各方 `shmdt` → 回收。

---

## 48.9 限额 · 运维

| 参数 | 含义 |
|------|------|
| `shmmax` | 单段最大字节 |
| `shmmni` | 段个数上限 |
| `shmall` | 总页数上限 |

`ipcs -m` · `ipcrm -m id` / `-M key`

---

## 优缺点

✅ 无拷贝、多进程同区。  
❌ 非 fd、无 epoll「数据就绪」、须自管同步、内核持久易漏、API 老。  
→ 新项目常选 **POSIX shm + mmap**。

拷贝对比：pipe/mq ≈ 两次拷贝；shm ≈ **零业务拷贝**（仍有页表建立成本）。

---

## SysV IPC 速记（四章收束）

| | mq | sem | shm |
|--|----|-----|-----|
| 句柄 | 非 fd | 非 fd | 非 fd |
| 持久 | 内核 | 内核 | 内核 |
| RMID | 立即标记 | 立即标记 | **等 nattch=0** |
| 角色 | 消息 | 同步 | 数据 |

---

## 思考题要点

1. RMID 延迟到 nattch=0。  
2. attach/detach；exec 全 detach。  
3. 虚址不同 → offset。  
4. 无同步 → 竞态。  
5. attach++ / detach--；nattch=0 + DEST → 回收。  
6. pipe/mq 拷贝 vs shm 共享页。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 同物理页；无拷贝；须同步 |
| 2 | shmat(NULL)；exec 自动 detach |
| 3 | RMID 延迟；看 shm_nattch |
| 4 | 段内只用 offset |
| 5 | shm + sem 经典配对 |
| 6 | 新项目可 POSIX shm |

---

## 参考

- Kerrisk · TLPI Ch48（非「第 21 章」误标）  
- `man 2 shmget` · `shmat` · `shmdt` · `shmctl`
