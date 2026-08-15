# TLPI 第 32 章 — Threads: Thread Cancellation

**优先级**：🟠（可取消工作线程；资源与锁安全）  
**前置**：[Ch29](../chapter-29-threads-intro/README.md) · [Ch30](../chapter-30-thread-synchronization/README.md)  
**后置**：[Ch33 线程更多细节](../chapter-33-threads-further/README.md)

---

## 小节目录

- [32.1 –32.2 取消请求](notes/32.1-canceling-a-thread.md)
- [32.3 状态与类型](notes/32.2-cancellation-state-and-type.md)
- [32.4 取消点](notes/32.4-testing-for-thread-cancellation.md)
- [32.5 Cleanup handlers](./notes/32.5-cleanup-handlers.md)
- [32.6 生命周期（延迟）](notes/32.6-asynchronous-cancelability.md)

---

## 章节目标


`pthread_cancel`；取消状态/类型/取消点；cleanup 栈；延迟 vs 异步；join 识别 `PTHREAD_CANCELED`。

---


---

## 32.7 易错清单


1. 异步取消 + 持锁 → 锁可能永占  
2. 无 cleanup → 泄漏  
3. `return` 不跑 cleanup  
4. push/pop 不成对  
5. 长循环无 `testcancel` → 永不响应  
6. 临界段可临时 DISABLE  

---


---

## 实验清单


1. cancel + `PTHREAD_CANCELED`  
2. `testcancel`  
3. （选）临时 DISABLE  
4. cleanup 释放资源  
5. return vs cancel 对 cleanup 差异  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | cancel 只投递请求；默认延迟到取消点 |
| 2 | 工程用 DEFERRED；慎用 ASYNC |
| 3 | 计算循环插 `testcancel` |
| 4 | cleanup LIFO；push/pop 成对 |
| 5 | return 不跑 cleanup；exit/cancel 会跑 |
| 6 | join 看 `PTHREAD_CANCELED` |

---


---

## 参考


- Kerrisk · TLPI Ch32  
- `man 3 pthread_cancel` · `man 3 pthread_cleanup_push` · `man 3 pthread_testcancel`


---

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

/* Ch32 线程取消 — pthread_cancel/cleanup_push/cleanup_pop。
 * 演示取消线程 + 清理函数执行。
 * 编译: gcc -o ch32_demo ch32_demo.c -lpthread */

static int fd = -1;

void cleanup_handler(void *arg) {
    printf("Cleanup: %s\n", (char *)arg);
    if (fd >= 0) {
        close(fd);
        printf("Cleanup: fd closed\n");
    }
}

void *worker(void *arg) {
    /* 注册清理函数（LIFO 顺序执行） */
    pthread_cleanup_push(cleanup_handler, "step 1");
    pthread_cleanup_push(cleanup_handler, "step 2");

    fd = open("/tmp/ch32_test.txt", O_WRONLY | O_CREAT, 0644);
    printf("Worker: opened fd=%d, entering loop...\n", fd);

    /* 默认延迟取消点: sleep 是取消点 */
    while (1) {
        printf("Worker: working...\n");
        sleep(1);  /* 取消点: 如果被取消，在此触发 */
    }

    /* cleanup_push 和 cleanup_pop 必须配对 */
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    return NULL;
}

int main(void) {
    pthread_t tid;
    pthread_create(&tid, NULL, worker, NULL);

    sleep(3);
    printf("Main: cancelling worker thread...\n");
    pthread_cancel(tid);
    pthread_join(tid, NULL);
    printf("Main: worker joined after cancellation\n");

    if (fd >= 0) close(fd);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
