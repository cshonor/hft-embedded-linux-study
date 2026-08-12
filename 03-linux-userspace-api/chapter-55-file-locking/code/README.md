# Ch55 demos — file locking

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o pidfile_lock pidfile_lock.c
# terminal A
./pidfile_lock /tmp/ch55.pid
# terminal B (should fail while A holds lock)
./pidfile_lock /tmp/ch55.pid

gcc -Wall -Wextra -o flock_demo flock_demo.c
./flock_demo /tmp/ch55.flock
```

| 文件 | 说明 |
|------|------|
| `pidfile_lock.c` | `fcntl` `F_WRLCK` 单实例 pid 文件 |
| `flock_demo.c` | `LOCK_EX\|LOCK_NB` 整文件锁 |

## 代码示例

```c

/* flock vs fcntl 文件锁对比 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

int main(void) {
    int fd = open("/tmp/ch55_lock.txt", O_RDWR | O_CREAT, 0644);

    /* 1) flock: 整文件锁（BSD 风格，仅咨询锁） */
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        perror("flock LOCK_EX");
    } else {
        printf("flock: exclusive lock acquired\n");
        write(fd, "flock write\n", 12);
        flock(fd, LOCK_UN);
        printf("flock: unlocked\n");
    }

    /* 2) fcntl: 字节范围锁（POSIX 标准） */
    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 10  /* 锁定前 10 字节 */
    };
    if (fcntl(fd, F_SETLK, &fl) < 0) {
        perror("fcntl F_SETLK");
    } else {
        printf("fcntl: write lock on bytes 0-9\n");
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        printf("fcntl: unlocked\n");
    }

    close(fd);
    return 0;
}

```

---
