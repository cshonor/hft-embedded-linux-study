# TLPI 第 31 章 — Thread Safety and Per-Thread Storage

**优先级**：🔴（并发库函数、无锁线程私有上下文）  
**前置**：[Ch30 同步](../chapter-30-thread-synchronization/README.md)  
**后置**：[Ch32 线程取消](../chapter-32-thread-cancellation/README.md)

---

## 小节目录

- [31.1 线程安全与可重入](notes/31.1-thread-safety-and-reentrancy-revisited.md)
- [31.2 `pthread_once`](notes/31.2-one-time-initialization.md)
- [31.3 TSD：`pthread_key_*`（核心）](notes/31.3-thread-specific-data.md)
- [31.4 静态 TLS：`__thread` / `_Thread_local`](notes/31.4-thread-local-storage.md)
- [31.5 易错 / 原理](notes/31.5-summary.md)

---

## 章节目标


分清线程安全 / 可重入；`pthread_once`；TSD 四 API 与 strerror 改造范式；对比 `__thread` TLS；关联 C++/`errno`/嵌入式与低延迟选型。

---


---

## 与 C++ / 嵌入式 / HFT


| 栈 | |
|----|--|
| C++ | `thread_local` ≈ TLS；可直接调 pthread；**勿混**两套线程生命周期模型 |
| 用户态 only | 内核驱动无 POSIX TSD |
| HFT | `__thread` 放连接/缓冲，少锁、少抖动；禁 `strtok`/`localtime` 一类 |

---


---

## 实验清单


1. TSD 版「线程安全缓冲」  
2. `__thread` 对比  
3. （选）静态缓冲多线程错乱复现  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 可重入 ⊂ 更强；线程安全可用锁/TLS |
| 2 | 用 `_r` 或自带缓冲，别用静态内部状态 API |
| 3 | `pthread_once` + `key_create` 改造老接口 |
| 4 | key 全局、值每线程；退出跑 destructor |
| 5 | `key_delete` 不析构各线程数据 |
| 6 | 低延迟优先 `__thread` / `thread_local` |

---


---

## 参考


- Kerrisk · TLPI Ch31  
- `man 3 pthread_once` · `man 3 pthread_key_create` · `man 3 pthread_getspecific`


---

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* Ch31 线程安全与线程特定数据 (TSD)。
 * 演示 strerror_r 线程安全版 + pthread_key 线程局部存储。
 * 编译: gcc -o ch31_demo ch31_demo.c -lpthread */

static pthread_key_t tsd_key;

/* 线程特定数据: 每个线程有独立的副本 */
void tsd_destructor(void *value) {
    printf("TSD destructor: freeing %s\n", (char *)value);
    free(value);
}

void *worker(void *arg) {
    int id = *(int *)arg;

    /* 线程安全函数: strerror_r 而非 strerror */
    char errbuf[256];
    strerror_r(ENOENT, errbuf, sizeof(errbuf));
    printf("Thread %d: strerror_r(ENOENT) = %s\n", id, errbuf);

    /* 设置线程特定数据 */
    char *msg = malloc(64);
    snprintf(msg, 64, "TSD data for thread %d", id);
    pthread_setspecific(tsd_key, msg);

    /* 读取线程特定数据 */
    char *my_data = pthread_getspecific(tsd_key);
    printf("Thread %d: TSD = %s\n", id, my_data);

    return NULL;
}

int main(void) {
    pthread_key_create(&tsd_key, tsd_destructor);

    pthread_t t1, t2;
    int id1 = 1, id2 = 2;
    pthread_create(&t1, NULL, worker, &id1);
    pthread_create(&t2, NULL, worker, &id2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_key_delete(tsd_key);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
