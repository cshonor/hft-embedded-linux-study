# TLPI 第 51 章 — Introduction to POSIX IPC

**优先级**：🟡（POSIX 三件套地图；对标 SysV）  
**前置**：[Ch45–48 SysV](../chapter-45-sysv-ipc-intro/notes.md) · [Ch49–50 mmap/VM](../chapter-49-memory-mappings/notes.md)  
**后置**：[Ch52 mq](../chapter-52-posix-message-queues/notes.md) → [Ch53 sem](../chapter-53-posix-semaphores/notes.md) → [Ch54 shm](../chapter-54-posix-shared-memory/notes.md)

---

## 小节目录

- [51.1 三类机制](./notes/51.1-mechanism.md)
- [51.2 统一模型](./notes/51.2-model.md)
- [51.3 POSIX vs System V（核心表）](./notes/51.3-system.md)
- [51.4 三件预览](./notes/51.4-section-51-4.md)

---

## 章节目标


三类 POSIX IPC；文件风 API 与引用计数；vs SysV 总表；选型。

---


---

## 选型（TLPI 倾向）


1. 新 Linux → **优先 POSIX**；事件驱动 → POSIX mq  
2. 老 UNIX / 遗留 → SysV  
3. 大批量 → shm + POSIX sem  
4. 简单流 → pipe / UNIX 域 socket  

---


---

## 思考题要点


1. unlink 删名；close 降引用；全 close 销毁。  
2. mq 返回 fd；SysV 非 fd。  
3. 命名：无关进程；匿名：线程/共享区。  
4. SysV 集可原子多 op；POSIX 单计数器更简单。  
5. unlink 后已打开句柄仍有效。  
6. shm_open → ftruncate → mmap。

---


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


---

## 参考


- Kerrisk · TLPI Ch51（非「第 31 章」误标）  
- `man 7 mq_overview` · `sem_overview` · `shm_overview`


---

## 代码示例

```c
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>

/* Ch51 POSIX IPC 概述 — 命名规则 + mq/sem/shm 统一接口。
 * POSIX IPC 用名字(类似文件路径)而非 key。
 * 编译: gcc -o ch51_demo ch51_demo.c -lrt -lpthread */

int main(void) {
    /* POSIX IPC 命名规则:
     * - 以 / 开头, 中间无其他 /
     * - 如 /myqueue, /mysem, /myshm
     * - 存储位置: /dev/shm/ (shared memory + semaphores)
     */

    /* POSIX 消息队列 */
    struct mq_attr attr = { .mq_maxmsg = 10, .mq_msgsize = 64 };
    mqd_t mqd = mq_open("/ch51_test", O_CREAT | O_RDWR, 0644, &attr);
    if (mqd >= 0) {
        printf("POSIX message queue created: /ch51_test\n");
        mq_close(mqd);
        mq_unlink("/ch51_test");
    }

    /* POSIX 信号量 */
    sem_t *sem = sem_open("/ch51_test", O_CREAT, 0644, 1);
    if (sem != SEM_FAILED) {
        printf("POSIX semaphore created: /ch51_test\n");
        sem_close(sem);
        sem_unlink("/ch51_test");
    }

    /* POSIX 共享内存 */
    int fd = shm_open("/ch51_test", O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        ftruncate(fd, 4096);
        printf("POSIX shared memory created: /ch51_test (size=4096)\n");
        close(fd);
        shm_unlink("/ch51_test");
    }

    printf("\nPOSIX IPC vs SysV IPC:\n");
    printf("  Naming:   name-based vs key-based\n");
    printf("  Cleanup:  _unlink vs IPC_RMID\n");
    printf("  Listing:  ls /dev/shm vs ipcs\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
