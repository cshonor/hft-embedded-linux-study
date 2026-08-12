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

## 代码示例

```c
#include <stdio.h>
#include <sys/sem.h>
/* Ch47 demo: semget + semop */
int main(void) {
    int id = semget(IPC_PRIVATE, 1, IPC_CREAT|0666);
    semctl(id, 0, SETVAL, 1);
    struct sembuf s = {0, -1, 0}; semop(id, &s, 1);
    printf("P done: %d\n", semctl(id, 0, GETVAL));
    s.sem_op = 1; semop(id, &s, 1);
    printf("V done: %d\n", semctl(id, 0, GETVAL));
    semctl(id, 0, IPC_RMID);
    return 0;
}
```

---
