# Ch46 demos — System V message queues

在 Linux / WSL 下于本目录执行：

```bash
touch /tmp/ch46-ftok-token
gcc -Wall -Wextra -o mq_demo mq_demo.c
./mq_demo /tmp/ch46-ftok-token

# leftovers
ipcs -q
# ipcrm -q <msqid>
```

| 文件 | 说明 |
|------|------|
| `mq_demo.c` | 创建队列 → 按 mtype 发送 → `msgtyp` 筛选接收 → `IPC_RMID` |
