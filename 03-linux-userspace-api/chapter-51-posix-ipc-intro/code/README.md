# Ch51 demos — POSIX IPC naming / lifecycle

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o posix_name_demo posix_name_demo.c -lrt
./posix_name_demo
# inspect: ls -l /dev/shm
```

| 文件 | 说明 |
|------|------|
| `posix_name_demo.c` | `shm_open` → close → unlink（导论生命周期） |

## 代码示例

```c

/* POSIX IPC 命名规则演示 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    /* POSIX 消息队列：名字以 / 开头，不超过 NAME_MAX */
    mqd_t mq = mq_open("/test_mq", O_CREAT | O_RDWR, 0644, NULL);
    if (mq != (mqd_t)-1) {
        printf("mq_open /test_mq OK\n");
        mq_close(mq);
        mq_unlink("/test_mq");
    }

    /* POSIX 信号量 */
    sem_t *sem = sem_open("/test_sem", O_CREAT, 0644, 1);
    if (sem != SEM_FAILED) {
        printf("sem_open /test_sem OK\n");
        sem_close(sem);
        sem_unlink("/test_sem");
    }

    /* POSIX 共享内存 */
    int fd = shm_open("/test_shm", O_CREAT | O_RDWR, 0644);
    if (fd >= 0) {
        ftruncate(fd, 4096);
        printf("shm_open /test_shm OK\n");
        close(fd);
        shm_unlink("/test_shm");
    }

    return 0;
}

```

---
