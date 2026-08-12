# Ch49 demos — mmap

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o mmap_anon mmap_anon.c
./mmap_anon

gcc -Wall -Wextra -o mmap_file mmap_file.c
./mmap_file /tmp/ch49-mmap.dat
```

| 文件 | 说明 |
|------|------|
| `mmap_anon.c` | `MAP_SHARED\|MAP_ANONYMOUS` 父子 IPC |
| `mmap_file.c` | `MAP_SHARED` 文件映射；父写子读 + `msync` |

## 代码示例

```c

/* mmap_anon.c — MAP_SHARED|MAP_ANONYMOUS 父子 IPC */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    /* 匿名共享映射：父子共享同一物理页 */
    char *shared = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) { perror("mmap"); exit(1); }
    strcpy(shared, "hello from parent");

    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程继承映射，看到父进程写入的数据 */
        printf("child reads: %s\n", shared);
        strcpy(shared, "hello from child");
        _exit(0);
    }
    wait(NULL);
    printf("parent reads after child: %s\n", shared);
    munmap(shared, 4096);
    return 0;
}

```

---
