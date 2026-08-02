# Ch52 demos — POSIX message queues

在 Linux / WSL 下于本目录执行（需 `/dev/mqueue`）：

```bash
gcc -Wall -Wextra -o mq_prio_demo mq_prio_demo.c -lrt
./mq_prio_demo

# leftovers
# ls /dev/mqueue
# rm /dev/mqueue/ch52_demo   # or mq_unlink from code
```

| 文件 | 说明 |
|------|------|
| `mq_prio_demo.c` | 创建队列、按优先级发送、receive 取高优先级、unlink |
