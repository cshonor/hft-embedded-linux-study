# Ch23 demos — Timers and Sleeping

```bash
cc -Wall -Wextra -o nanosleep_retry nanosleep_retry.c
./nanosleep_retry
# optional: press Ctrl+C during sleep to see EINTR retry

cc -Wall -Wextra -o posix_timer_thread posix_timer_thread.c -lrt
./posix_timer_thread
# fires a few times via SIGEV_THREAD, then exits
```

| 文件 | 说明 |
|------|------|
| `nanosleep_retry.c` | `nanosleep` + `EINTR` 安全重试 |
| `posix_timer_thread.c` | `timer_create` + `CLOCK_MONOTONIC` + `SIGEV_THREAD` |
