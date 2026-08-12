# Ch53 demos — POSIX semaphores

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o named_sem_demo named_sem_demo.c -pthread
./named_sem_demo

gcc -Wall -Wextra -o anon_sem_demo anon_sem_demo.c -pthread
./anon_sem_demo
```

| 文件 | 说明 |
|------|------|
| `named_sem_demo.c` | `/name` 二元信号量 wait/post + unlink |
| `anon_sem_demo.c` | `MAP_SHARED` + `sem_init(pshared=1)` 父子握手 |

## 代码示例

```c

/* POSIX 信号量：命名信号量 sem_open + sem_wait/sem_post */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    /* 创建命名信号量，初值为 1（可用） */
    sem_t *sem = sem_open("/demo_sem", O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); exit(1); }

    /* fork 后父子共享同一信号量 */
    pid_t pid = fork();
    if (pid == 0) {
        sem_wait(sem);  /* P 操作 */
        printf("child: entered critical section\n");
        sleep(1);
        printf("child: leaving\n");
        sem_post(sem);  /* V 操作 */
        _exit(0);
    }

    sem_wait(sem);
    printf("parent: entered critical section\n");
    sleep(1);
    printf("parent: leaving\n");
    sem_post(sem);

    wait(NULL);
    sem_close(sem);
    sem_unlink("/demo_sem");
    return 0;
}

```

---
