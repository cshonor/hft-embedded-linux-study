# Ch33 demos — Threads further details

```bash
cc -Wall -Wextra -pthread -o thread_sigwait thread_sigwait.c
./thread_sigwait
# other shell: kill -USR1 <pid> ; kill -TERM <pid>

cc -Wall -Wextra -pthread -o pthread_barrier_demo pthread_barrier_demo.c
./pthread_barrier_demo

cc -Wall -Wextra -pthread -o pthread_rwlock_demo pthread_rwlock_demo.c
./pthread_rwlock_demo
```

| 文件 | 说明 |
|------|------|
| `thread_sigwait.c` | 阻塞信号 + 专用 `sigwait` 线程 |
| `pthread_barrier_demo.c` | 屏障集合点 |
| `pthread_rwlock_demo.c` | 读写锁基础 |

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
/* Ch33 demo: detached thread */
void *f(void *a) { sleep(1); printf("done\n"); return NULL; }
int main(void) {
    pthread_t t;
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    pthread_create(&t, &a, f, NULL);
    pthread_attr_destroy(&a);
    sleep(2);
    return 0;
}
```

---
