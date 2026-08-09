# Ch48 demos — System V shared memory + semaphore

在 Linux / WSL 下于本目录执行：

```bash
touch /tmp/ch48-ftok-token
gcc -Wall -Wextra -o shm_sem_demo shm_sem_demo.c
./shm_sem_demo /tmp/ch48-ftok-token

ipcs -m -s
# ipcrm -m <shmid>; ipcrm -s <semid>
```

| 文件 | 说明 |
|------|------|
| `shm_sem_demo.c` | 父写子读；二元 sem 握手；`IPC_RMID` 后双方 `shmdt` |
