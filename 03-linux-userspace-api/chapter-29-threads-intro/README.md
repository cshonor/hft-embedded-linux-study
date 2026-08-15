# TLPI 第 29 章 — Threads: Introduction

**优先级**：🔴（并发基础；下一章同步）  
**前置**：[Ch28 fork/exec 深潜](../chapter-28-process-creation-exec-detail/README.md)  
**后置**：[Ch30 线程同步](../chapter-30-thread-synchronization/README.md)

---

## 小节目录

- [29.1 概念](notes/29.1-overview.md)
- [29.2 Pthreads 规范](notes/29.2-background-details-of-the-pthreads-api.md)
- [29.4 线程 vs 进程](notes/29.4-thread-termination.md)

---

## 章节目标


建立 Pthreads 模型；分清共享/私有资源；掌握 create / join / detach / `pthread_exit`；认识竞争条件，为互斥锁铺垫。

---


---

## 29.3 核心 API


### 创建

```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
```

`attr==NULL` → 默认可接合。新线程可能在 `create` 返回前已跑。自身 ID：`pthread_self()`。

### 终止

| 方式 | |
|------|--|
| start 函数 `return` | |
| `pthread_exit(retval)` | |
| `pthread_cancel` | Ch32 |

❌ 线程内 **`exit()`** → 整个进程没。  
⚠️ 主线程从 `main` **return** ≈ `exit()` → 其它线程一并死；要等子线程时主线程用 `pthread_join` 或 `pthread_exit`。

### join / detach

```c
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
```

| | joinable | detached |
|--|----------|----------|
| 回收 | 须 `pthread_join` | 终止后自动回收 |
| 返回值 | 可取 | 不可 join |

同一线程只能 join 一次。可接合退出无人 join → **僵尸线程**。

### 属性（略）

`pthread_attr_init` / `destroy`；可设栈大小、分离状态等。

Demo：[`code/simple_thread.c`](./code/simple_thread.c) · [`code/thread_exit_retval.c`](./code/thread_exit_retval.c) · [`code/detached_thread.c`](./code/detached_thread.c) · [`code/thread_race.c`](./code/thread_race.c)

---


---

## 29.5 易错清单


1. 线程里 `exit()`  
2. 主线程 `return` 拖死其它线程  
3. 把**即将失效的栈地址**传给新线程  
4. 无同步读写共享变量 → 竞争（Ch30）  
5. 混淆 PID 与 `pthread_t`  
6. 忘 join → 僵尸线程  
7. 多线程 fork：子进程只留调用线程（Ch28）  

---


---

## 实验清单


1. create + join  
2. 独立 `errno` / 共享全局  
3. joinable vs detached  
4. （选）主线程 exit 拖死线程  
5. （选）栈指针传参野指针  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 共享地址空间；私有栈与 errno |
| 2 | `-pthread`；成功返回 0 |
| 3 | 禁 `exit`；用 return / `pthread_exit` |
| 4 | joinable 必须 join，否则僵尸 |
| 5 | detach 自动回收，不可再 join |
| 6 | 竞争 → 下一章互斥锁 |

---


---

## 参考


- Kerrisk · TLPI Ch29  
- `man 7 pthreads` · `man 3 pthread_create` · `man 3 pthread_join` · `man 3 pthread_detach`


---

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

/* Ch29 线程入门 — pthread_create/pthread_join。
 * 线程共享地址空间，比 fork 轻量。
 * 编译: gcc -o ch29_demo ch29_demo.c -lpthread */

static int shared_counter = 0;  /* 线程间共享 */

void *worker(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        shared_counter++;
        printf("Thread %d: counter=%d\n", id, shared_counter);
        usleep(100000);  /* 100ms */
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;

    pthread_create(&t1, NULL, worker, &id1);
    pthread_create(&t2, NULL, worker, &id2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Final counter: %d (expected 10)\n", shared_counter);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
