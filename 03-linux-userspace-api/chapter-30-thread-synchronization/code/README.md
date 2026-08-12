# Ch30 demos — Thread synchronization

```bash
cc -Wall -Wextra -pthread -o thread_incr_mutex thread_incr_mutex.c
./thread_incr_mutex
# expect counter == 200000

cc -Wall -Wextra -pthread -o prod_condvar prod_condvar.c
./prod_condvar

# race without lock: ../../chapter-29-threads-intro/code/thread_race.c
```

| 文件 | 说明 |
|------|------|
| `thread_incr_mutex.c` | mutex 保护累加（对齐 TLPI thread_incr_mutex） |
| `prod_condvar.c` | 有界缓冲 + cond 生产/消费 |

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
/* Ch30 demo: mutex */
static int n = 0;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
void *f(void *a) { for (int i=0;i<1000;i++) { pthread_mutex_lock(&m); n++; pthread_mutex_unlock(&m); } return NULL; }
int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, f, NULL);
    pthread_create(&t2, NULL, f, NULL);
    pthread_join(t1, NULL); pthread_join(t2, NULL);
    printf("n=%d\n", n);
    return 0;
}
```

---
