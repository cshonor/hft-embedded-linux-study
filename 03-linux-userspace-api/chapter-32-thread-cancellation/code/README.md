# Ch32 demos — Thread cancellation

```bash
cc -Wall -Wextra -pthread -o thread_cancel thread_cancel.c
./thread_cancel

cc -Wall -Wextra -pthread -o thread_testcancel thread_testcancel.c
./thread_testcancel

cc -Wall -Wextra -pthread -o thread_cleanup thread_cleanup.c
./thread_cleanup
```

| 文件 | 说明 |
|------|------|
| `thread_cancel.c` | cancel + join → `PTHREAD_CANCELED` |
| `thread_testcancel.c` | 忙循环中 `pthread_testcancel` |
| `thread_cleanup.c` | cleanup_push/pop 释放堆缓冲 |

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
/* Ch32 demo: pthread_cancel */
void *f(void *a) { while(1) sleep(1); return NULL; }
int main(void) {
    pthread_t t;
    pthread_create(&t, NULL, f, NULL);
    sleep(2);
    pthread_cancel(t);
    pthread_join(t, NULL);
    printf("cancelled\n");
    return 0;
}
```

---
