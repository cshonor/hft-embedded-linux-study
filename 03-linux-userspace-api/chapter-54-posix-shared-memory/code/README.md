# Ch54 demos — POSIX shared memory

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o posix_shm_demo posix_shm_demo.c -lrt
./posix_shm_demo

# leftovers: ls /dev/shm ; rm /dev/shm/ch54_demo
```

| 文件 | 说明 |
|------|------|
| `posix_shm_demo.c` | shm_open → ftruncate → mmap → 父写子读 → unlink |

## 代码示例

```c

/* POSIX 共享内存：shm_open + ftruncate + mmap */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    /* 创建共享内存对象 */
    int fd = shm_open("/demo_shm", O_CREAT | O_RDWR, 0644);
    if (fd < 0) { perror("shm_open"); exit(1); }

    /* 设置大小 */
    if (ftruncate(fd, 4096) < 0) { perror("ftruncate"); exit(1); }

    /* 映射 */
    char *shm = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); exit(1); }
    close(fd);  /* 映射后可关闭 fd */

    strcpy(shm, "hello from POSIX shm");
    printf("written: %s\n", shm);

    munmap(shm, 4096);
    shm_unlink("/demo_shm");
    return 0;
}

```

---
