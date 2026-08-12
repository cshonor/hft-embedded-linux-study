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

## 代码示例

```c
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
/* Ch46 demo: msgsnd + msgrcv */
int main(void) {
    int id = msgget(IPC_PRIVATE, IPC_CREAT|0666);
    struct { long t; char m[32]; } msg = {1, "hello"};
    msgsnd(id, &msg, strlen(msg.m)+1, 0);
    msgrcv(id, &msg, 32, 0, 0);
    printf("got: %s\n", msg.m);
    msgctl(id, IPC_RMID, NULL);
    return 0;
}
```

---
