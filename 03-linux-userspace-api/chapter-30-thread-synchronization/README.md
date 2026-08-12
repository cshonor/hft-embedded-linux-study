# TLPI 第 30 章 — Threads: Thread Synchronization

**优先级**：🔴（多线程正确性核心）  
**前置**：[Ch29 线程导论](../chapter-29-threads-intro/notes.md)  
**后置**：[Ch31 线程安全 / TLS](../chapter-31-thread-safety-tsd/notes.md)

---

## 小节目录

- [30.1 互斥量 `pthread_mutex_t`](./notes/30.1-pthreadmutext.md)
- [30.2 条件变量 `pthread_cond_t`](./notes/30.2-pthreadcondt.md)
- [30.3 职责与易错](./notes/30.3-section-30-3.md)

---

## 章节目标


竞态与临界区；`pthread_mutex_*`；`pthread_cond_*` + while 谓词；生产者-消费者；死锁与职责分离。

---


---

## 实验清单


1. 无锁累加错误（Ch29 `thread_race`）  
2. mutex 修复  
3. （选）`trylock`  
4. while vs if  
5. 生产-消费  
6. （选）多锁死锁  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 临界区用 mutex |
| 2 | cond 永远配 mutex |
| 3 | `while (!pred) wait` |
| 4 | wait = 放锁睡，醒再加锁 |
| 5 | 多锁统一顺序防死锁 |
| 6 | mutex 管状态，cond 管等待 |

---


---

## 参考


- Kerrisk · TLPI Ch30  
- `man 3 pthread_mutex_lock` · `man 3 pthread_cond_wait`


---

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/* Ch30 线程同步 — mutex/cond/rwlock。
 * 演示互斥锁保护共享数据 + 条件变量通知。
 * 编译: gcc -o ch30_demo ch30_demo.c -lpthread */

static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int ready = 0;

void *producer(void *arg) {
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        ready = 1;
        printf("Producer: counter=%d, signaling\n", counter);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
        usleep(200000);
    }
    return NULL;
}

void *consumer(void *arg) {
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&lock);
        while (!ready)
            pthread_cond_wait(&cond, &lock);  /* 原子释放锁+等待 */
        ready = 0;
        printf("Consumer: consumed counter=%d\n", counter);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    pthread_t pt, ct;
    pthread_create(&pt, NULL, producer, NULL);
    pthread_create(&ct, NULL, consumer, NULL);
    pthread_join(pt, NULL);
    pthread_join(ct, NULL);
    printf("Done: counter=%d\n", counter);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
