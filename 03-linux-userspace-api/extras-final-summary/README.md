# TLPI 附录 — Summary and Further Reading

**优先级**：⭐ / ⭐⭐ / ⭐⭐⭐（见根目录 [README.md](../README.md) 优先级表）  

---

## 小节目录

- [00.3 要点梳理](./notes/00.3-section-00-3.md)

---

## 1. 本章目标




---

## 2. 核心 API / syscall




---

## 4. C 示例摘要




---

## 5. Rust 对照（`std` / `libc` / crate）




---

## 6. 常见坑与面试点




---

## 7. 背诵卡


| # | 要点 |
|---|------|
| 1 | |

---


---

## 8. 参考


- 《The Linux Programming Interface》第 64 章 — Summary and Further Reading
- `man 2` / `man 3` / `man 7`


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

/* TLPI 全书总结 — 综合示例: fork + pipe + signal + thread + mmap。
 * 演示多个 TLPI 核心概念在一个程序中的协作。
 * 编译: gcc -o final_demo final_demo.c -lpthread */

static int *shared_counter;  /* mmap 共享内存 */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *thread_func(void *arg) {
    pthread_mutex_lock(&lock);
    (*shared_counter)++;
    pthread_mutex_unlock(&lock);
    return NULL;
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(void) {
    /* 1. mmap 共享内存 */
    shared_counter = mmap(NULL, sizeof(int),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *shared_counter = 0;

    /* 2. 管道通信 */
    int pipefd[2];
    pipe(pipefd);

    /* 3. 信号处理 */
    signal(SIGCHLD, sigchld_handler);

    /* 4. fork + exec 模型 */
    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        const char *msg = "child says hello";
        write(pipefd[1], msg, strlen(msg) + 1);
        close(pipefd[1]);
        _exit(0);
    }

    close(pipefd[1]);
    char buf[64];
    read(pipefd[0], buf, sizeof(buf));
    printf("Parent received: %s\n", buf);
    close(pipefd[0]);

    /* 5. 多线程 */
    pthread_t threads[3];
    for (int i = 0; i < 3; i++)
        pthread_create(&threads[i], NULL, thread_func, NULL);
    for (int i = 0; i < 3; i++)
        pthread_join(threads[i], NULL);

    printf("Shared counter after 3 threads: %d\n", *shared_counter);

    /* 6. 时间 */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("Monotonic time: %ld.%09ld\n", (long)ts.tv_sec, (long)ts.tv_nsec);

    munmap(shared_counter, sizeof(int));

    printf("\nTLPI concepts used: mmap, pipe, signal, fork, thread, mutex, time\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
