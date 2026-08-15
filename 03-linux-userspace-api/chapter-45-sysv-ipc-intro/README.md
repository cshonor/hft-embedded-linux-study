# TLPI 第 45 章 — Introduction to System V IPC

**优先级**：🟡（SysV 三机制共用模型）  
**前置**：[Ch44 管道与 FIFO](../chapter-44-pipes-fifos/README.md)  
**后置**：[Ch46 SysV 消息队列](../chapter-46-sysv-message-queues/README.md) → [Ch47 信号量](../chapter-47-sysv-semaphores/README.md) → [Ch48 共享内存](../chapter-48-sysv-shared-memory/README.md)

---

## 小节目录

- [45.1 三类对象 · 统一 API](notes/45.1-api-overview.md)
- [45.2 Key（`key_t`）](notes/45.2-ipc-keys.md)
- [45.3 –45.4 标识符 · `ipc_perm`](notes/45.3-associated-data-structure-and-object-per.md)
- [45.5 `get()` 算法（高频）](notes/45.5-algorithm-employed-by-system-v-ipc-get-c.md)
- [45.6 内核持久 · 运维](notes/45.6-the-ipcs-and-ipcrm-commands.md)
- [45.7 –45.8 缺陷与限额](notes/45.7-obtaining-a-list-of-all-ipc-objects.md)
- [45.9 `IPC_RMID` 差异](notes/45.9-summary.md)

---

## 章节目标


三类对象共用的 key/`get`/`ctl`/操作 API；`ipc_perm`；`get` 算法；内核持久与 `ipcs`/`ipcrm`；限额；`IPC_RMID` 差异；与 POSIX 对比铺垫。

---


---

## 思考题要点


1. `IPC_PRIVATE`：亲缘；`ftok`：无关进程约定路径。  
2. `ftok`：固定路径文件勿删；或固定 key + 约定文档。  
3. key≠id；id 非 fd → 无 epoll。  
4. `CREAT|EXCL`：独占创建。  
5. 内核持久 + 忘 RMID → 泄漏占限额。  
6. shm 的 RMID 须等 detach。

---


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


---

## 参考


- Kerrisk · TLPI **Ch45**（非中文分册「第 10 章」）  
- `man 3 ftok` · `man 1 ipcs` · `man 1 ipcrm`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>

/* Ch45 SysV IPC 概述 — ftok 生成 key + ipcs/ipcrm 管理。
 * SysV IPC 三件套: msgget/semget/shmget，用 key 标识。
 * 编译: gcc -o ch45_demo ch45_demo.c */

int main(void) {
    /* ftok: 用文件路径 + 项目号生成 IPC key */
    /* 需要一个存在的文件 */
    int fd = open("/tmp/ch45_ftok_file", O_CREAT | O_RDWR, 0644);
    close(fd);

    key_t key = ftok("/tmp/ch45_ftok_file", 'A');
    printf("ftok key = 0x%x\n", (unsigned)key);

    /* 创建消息队列 (只是演示创建+删除) */
    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid >= 0) {
        printf("Message queue created: id=%d\n", msqid);
        struct msqid_ds info;
        msgctl(msqid, IPC_STAT, &info);
        printf("  queue size: %lu bytes\n", (unsigned long)info.msg_cbytes);
        printf("  messages:   %lu\n", (unsigned long)info.msg_qnum);
        msgctl(msqid, IPC_RMID, NULL);  /* 删除 */
        printf("  (deleted)\n");
    }

    /* 创建信号量集 */
    int semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid >= 0) {
        printf("Semaphore set created: id=%d\n", semid);
        semctl(semid, 0, IPC_RMID);
        printf("  (deleted)\n");
    }

    /* 创建共享内存 */
    int shmid = shmget(key, 4096, IPC_CREAT | 0666);
    if (shmid >= 0) {
        printf("Shared memory created: id=%d, size=4096\n", shmid);
        shmctl(shmid, IPC_RMID, NULL);
        printf("  (deleted)\n");
    }

    remove("/tmp/ch45_ftok_file");
    printf("\nUse 'ipcs' to list, 'ipcrm' to remove IPC objects\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
