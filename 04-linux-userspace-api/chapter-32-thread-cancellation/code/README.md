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
