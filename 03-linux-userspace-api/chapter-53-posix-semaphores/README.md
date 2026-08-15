# TLPI 第 53 章 — POSIX Semaphores

**优先级**：🔴（进程/线程同步）  
**前置**：[Ch52 POSIX mq](../chapter-52-posix-message-queues/README.md) · [Ch47 SysV sem](../chapter-47-sysv-semaphores/README.md)  
**后置**：[Ch54 POSIX 共享内存](../chapter-54-posix-shared-memory/README.md)

---

## 小节目录

- [53.1 概念](notes/53.1-overview.md)
- [53.2 命名](notes/53.2-named-semaphores.md)
- [53.3 操作（两形态共用）](notes/53.3-semaphore-operations.md)
- [53.4 匿名](notes/53.4-unnamed-semaphores.md)
- [53.5 fork / exec](notes/53.5-comparisons-with-other-synchronization-t.md)
- [53.6 vs SysV · vs mutex](notes/53.6-semaphore-limits.md)

---

## 章节目标


命名 / 匿名；wait/post/timed；fork/exec；vs SysV / mutex。

---


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


---

## 参考


- Kerrisk · TLPI Ch53  
- `man 3 sem_overview` · `sem_open` · `sem_init`


---

## 代码示例

```c
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

/* Ch53 POSIX 信号量 — sem_open/sem_wait/sem_post/sem_unlink。
 * 命名信号量可在无关进程间共享; 无名信号量在共享内存中使用。
 * 编译: gcc -o ch53_demo ch53_demo.c -lpthread */

#define SEM_NAME "/ch53_demo"

int main(void) {
    /* 创建命名信号量, 初始值=1 (可用) */
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); return 1; }

    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程: 获取信号量 */
        printf("Child: waiting for semaphore...\n");
        sem_wait(sem);   /* P: value 1->0, 如果为0则阻塞 */
        printf("Child: acquired, working 2s...\n");
        sleep(2);
        printf("Child: releasing\n");
        sem_post(sem);   /* V: value 0->1 */
        _exit(0);
    }

    sleep(1);
    /* 父进程: 也会被阻塞直到子进程释放 */
    printf("Parent: waiting for semaphore...\n");
    sem_wait(sem);   /* 会阻塞 */
    printf("Parent: acquired after child released\n");
    sem_post(sem);

    waitpid(pid, NULL, 0);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    /* 无名信号量 (进程内线程同步) */
    printf("\nUnnamed semaphore (for threads, in shared memory):\n");
    sem_t unnamed;
    sem_init(&unnamed, 0, 1);
    sem_wait(&unnamed);
    printf("  acquired unnamed semaphore\n");
    sem_post(&unnamed);
    sem_destroy(&unnamed);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
