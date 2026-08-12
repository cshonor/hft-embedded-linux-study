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

## 代码示例

```c
#include <stdio.h>
#include <sys/shm.h>
#include <string.h>
/* Ch48 demo: shmget + shmat */
int main(void) {
    int id = shmget(IPC_PRIVATE, 4096, IPC_CREAT|0666);
    char *s = shmat(id, NULL, 0);
    strcpy(s, "shared!");
    printf("%s\n", s);
    shmdt(s); shmctl(id, IPC_RMID, NULL);
    return 0;
}
```

---
