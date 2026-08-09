# Ch29 demos — Threads introduction

```bash
cc -Wall -Wextra -pthread -o simple_thread simple_thread.c
./simple_thread

cc -Wall -Wextra -pthread -o thread_exit_retval thread_exit_retval.c
./thread_exit_retval

cc -Wall -Wextra -pthread -o detached_thread detached_thread.c
./detached_thread

cc -Wall -Wextra -pthread -o thread_race thread_race.c
./thread_race
# often prints a total != 200000 (data race demo)
```

| 文件 | 说明 |
|------|------|
| `simple_thread.c` | create + join |
| `thread_exit_retval.c` | `pthread_exit` / return 传回值 |
| `detached_thread.c` | detach 后自动回收 |
| `thread_race.c` | 无锁累加竞争（铺垫 Ch30） |
