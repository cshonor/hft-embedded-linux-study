# TLPI 第 33 章 — Threads: Further Details

**优先级**：🔴（信号×线程、fork、NPTL 认知）  
**前置**：[Ch29](../chapter-29-threads-intro/notes.md)–[Ch32](../chapter-32-thread-cancellation/notes.md) · [Ch22 sigwait](../chapter-22-signals-advanced/notes.md) · [Ch28 fork](../chapter-28-process-creation-exec-detail/notes.md)  
**后置**：[Ch34 进程组/会话](../chapter-34-process-groups-sessions/notes.md)（线程模块收束；daemon 见 [Ch37](../chapter-37-daemons/notes.md)）

---

## 小节目录

- [33.1 线程栈](./notes/33.1-thread-stack.md)
- [33.2 线程与信号（重难点）](./notes/33.2-thread-signal.md)
- [33.3 进程控制](./notes/33.3-process.md)
- [33.4 实现模型](./notes/33.4-model.md)
- [33.5 LinuxThreads vs NPTL](./notes/33.5-linuxthreads-nptl.md)
- [33.6 高级同步（简介）](./notes/33.6-sync.md)

---

## 章节目标


线程栈属性；多线程信号模型；fork/exec/exit；`pthread_atfork`；M:1/1:1/M:N 与 NPTL；读写锁/屏障/自旋锁速览。

---


---

## 易错清单


1. 多线程改掩码用 **`pthread_sigmask`**，勿靠 `sigprocmask`  
2. 主线程 `return`/`exit` 杀光线程  
3. 多线程 fork → 立刻 exec  
4. `sigwait` 优于满地异步 handler  
5. `pthread_kill` 不能跨进程  
6. NPTL = 1:1；TID ≠ `pthread_t`  

---


---

## 实验清单


1. （选）`attr` 设栈大小  
2. `sigwait` 信号线程  
3. （选）`pthread_kill`  
4. barrier / rwlock  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | handler 共享；掩码每线程 |
| 2 | 阻塞信号 + `sigwait` 专用线程 |
| 3 | fork 只留一线程；立刻 exec |
| 4 | exit 杀进程；主线程可 `pthread_exit` |
| 5 | Linux = NPTL 1:1 |
| 6 | rwlock / barrier / spin 按场景用 |

---


---

## 参考


- Kerrisk · TLPI Ch33  
- `man 3 pthread_sigmask` · `man 3 pthread_kill` · `man 3 sigwait` · `man 3 pthread_atfork` · `man 7 pthreads`


---

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

/* Ch33 线程深入 — pthread_attr/线程栈/信号掩码。
 * 演示自定义线程属性: 栈大小 + 分离状态 + 信号掩码。
 * 编译: gcc -o ch33_demo ch33_demo.c -lpthread */

void *worker(void *arg) {
    int id = *(int *)arg;
    printf("Thread %d: running (stack custom sized)\n", id);
    /* 检查线程栈位置 */
    int local;
    printf("Thread %d: stack near %p\n", id, (void *)&local);
    return NULL;
}

int main(void) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    /* 设置分离状态: 线程结束自动回收，不需 join */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* 设置栈大小: 默认 8MB，改为 2MB */
    size_t stacksize;
    pthread_attr_getstacksize(&attr, &stacksize);
    printf("Default stack size: %zu bytes\n", stacksize);
    pthread_attr_setstacksize(&attr, 2 * 1024 * 1024);

    /* 设置线程不继承父进程的信号掩码 */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_attr_setsigmask_np(&attr, &mask);

    /* 创建线程 */
    int id = 42;
    pthread_t tid;
    pthread_create(&tid, &attr, worker, &id);

    pthread_attr_destroy(&attr);

    /* 分离状态线程不能 join，用 sleep 等待 */
    sleep(1);
    printf("Main: detached thread finished\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
