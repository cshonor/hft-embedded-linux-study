# TLPI 第 47 章 — System V Semaphores

**优先级**：🔴（同步原语；配共享内存）  
**前置**：[Ch45 导论](../chapter-45-sysv-ipc-intro/README.md) · [Ch46 消息队列](../chapter-46-sysv-message-queues/README.md)  
**后置**：[Ch48 SysV 共享内存](../chapter-48-sysv-shared-memory/README.md)

---

## 小节目录

- [47.1 概念](notes/47.1-overview.md)
- [47.2 –47.4 API](notes/47.2-creating-or-opening-a-semaphore-set.md)
- [47.5 初始化竞态（经典坑）](notes/47.5-semaphore-initialization.md)
- [47.8 `SEM_UNDO`](notes/47.8-semaphore-undo-values.md)
- [47.9 二元信号量 ≈ 互斥](notes/47.10-semaphore-limits.md)
- [47.10 –47.11 限额 · 缺陷](notes/47.10-semaphore-limits.md)

---

## 章节目标


信号量集；`semget`/`semctl`/`semop`；初始化竞态；`SEM_UNDO`；二元互斥；限额与缺陷。

---


---

## 思考题要点


1. 上节安全初始化。  
2. `SEM_UNDO` 兜底 ≠ 防死锁；无超时仍可永久阻塞。  
3. 多 `sembuf` 原子 → 减死锁。  
4. `IPC_RMID` → 阻塞 `semop` 失败返回（`EIDRM`）。  
5. 无所有者 vs mutex。

---


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


---

## 参考


- Kerrisk · TLPI Ch47（非「第 20 章」误标）  
- `man 2 semget` · `semop` · `semctl`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>

/* Ch47 SysV 信号量 — semget/semop/semctl。
 * 信号量用于同步: P 操作(等待) / V 操作(释放)。
 * 编译: gcc -o ch47_demo ch47_demo.c */

/* SysV 信号量是集合, 操作比较复杂 */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void sem_op(int semid, int op) {
    struct sembuf sb = {
        .sem_num = 0,
        .sem_op = op,    /* -1 = P (wait), +1 = V (signal) */
        .sem_flg = 0
    };
    semop(semid, &sb, 1);
}

int main(void) {
    /* 创建包含 1 个信号量的集合 */
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid < 0) { perror("semget"); return 1; }

    /* 初始化信号量值为 1 (可用) */
    union semun arg;
    arg.val = 1;
    semctl(semid, 0, SETVAL, arg);

    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程: P 操作 (获取信号量) */
        printf("Child: waiting for semaphore...\n");
        sem_op(semid, -1);  /* P: value 1->0 */
        printf("Child: got semaphore, working...\n");
        sleep(2);
        printf("Child: releasing semaphore\n");
        sem_op(semid, +1);  /* V: value 0->1 */
        _exit(0);
    }

    sleep(1);
    /* 父进程: 也尝试 P 操作, 会被阻塞直到子进程 V */
    printf("Parent: waiting for semaphore...\n");
    sem_op(semid, -1);  /* 会阻塞 */
    printf("Parent: got semaphore after child released\n");
    sem_op(semid, +1);

    waitpid(pid, NULL, 0);
    semctl(semid, 0, IPC_RMID);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
