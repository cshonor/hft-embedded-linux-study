# Ch31 demos — Thread safety / TSD / TLS

```bash
cc -Wall -Wextra -pthread -o strerror_tsd strerror_tsd.c
./strerror_tsd

cc -Wall -Wextra -pthread -o thread_local_buf thread_local_buf.c
./thread_local_buf

cc -Wall -Wextra -pthread -o static_buf_race static_buf_race.c
./static_buf_race
# shared static buffer → garbled / mixed output
```

| 文件 | 说明 |
|------|------|
| `strerror_tsd.c` | `pthread_once` + TSD 每线程缓冲 |
| `thread_local_buf.c` | `__thread` 静态 TLS |
| `static_buf_race.c` | 无 TLS 静态缓冲错乱（对照） |

## 代码示例

```c
#include <stdio.h>
#include <pthread.h>
#include <string.h>
/* Ch31 demo: strerror_r (线程安全) */
int main(void) {
    char buf[256];
    strerror_r(ENOENT, buf, sizeof(buf));
    printf("error: %s\n", buf);
    return 0;
}
```

---
