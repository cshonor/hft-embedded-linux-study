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
