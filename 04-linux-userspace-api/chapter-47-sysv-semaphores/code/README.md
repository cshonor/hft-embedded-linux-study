# Ch47 demos — System V semaphores

在 Linux / WSL 下于本目录执行：

```bash
touch /tmp/ch47-ftok-token
gcc -Wall -Wextra -o sem_demo sem_demo.c
./sem_demo /tmp/ch47-ftok-token

ipcs -s
# ipcrm -s <semid>   # if demo fails mid-way
```

| 文件 | 说明 |
|------|------|
| `sem_demo.c` | `CREAT\|EXCL` 安全初始化 + 二元 P/V + `SEM_UNDO` + `IPC_RMID` |
