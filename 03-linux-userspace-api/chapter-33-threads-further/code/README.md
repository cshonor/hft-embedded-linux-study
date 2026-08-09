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
