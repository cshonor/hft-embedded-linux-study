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

## 代码示例

```c

/* POSIX 消息队列：mq_open + mq_send + mq_receive */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>

int main(void) {
    struct mq_attr attr = { .mq_maxmsg = 10, .mq_msgsize = 256 };
    mqd_t mq = mq_open("/demo_mq", O_CREAT | O_RDWR, 0644, &attr);
    if (mq == (mqd_t)-1) { perror("mq_open"); exit(1); }

    /* 发送消息 */
    unsigned int prio = 5;
    mq_send(mq, "hello mq", 9, prio);
    printf("sent: 'hello mq' (prio=%u)\n", prio);

    /* 接收消息 */
    char buf[256];
    unsigned int recv_prio;
    ssize_t n = mq_receive(mq, buf, sizeof(buf), &recv_prio);
    if (n >= 0) {
        buf[n] = '\0';
        printf("received: '%s' (prio=%u, len=%zd)\n", buf, recv_prio, n);
    }

    mq_close(mq);
    mq_unlink("/demo_mq");
    return 0;
}

```

---
