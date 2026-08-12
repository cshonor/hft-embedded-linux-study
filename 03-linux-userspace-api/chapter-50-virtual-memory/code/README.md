# Ch50 demos — virtual memory ops

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o vm_ops_demo vm_ops_demo.c
./vm_ops_demo

# if mlock fails: raise lockable memory, e.g.
# ulimit -l unlimited   # or a larger soft limit
```

| 文件 | 说明 |
|------|------|
| `vm_ops_demo.c` | anon mmap → mprotect → mincore → madvise → mlock（可失败） |

## 代码示例

```c

/* mprotect + madvise + mincore 虚拟内存操作 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    size_t len = 4096;
    char *p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); exit(1); }
    strcpy(p, "writable now");
    printf("before mprotect: %s\n", p);

    /* 改为只读 */
    if (mprotect(p, len, PROT_READ) < 0) { perror("mprotect"); exit(1); }
    printf("mprotect -> PROT_READ\n");

    /* mincore 检查页是否在内存中 */
    unsigned char vec = 0;
    if (mincore(p, len, &vec) < 0) { perror("mincore"); exit(1); }
    printf("page resident: %s\n", (vec & 1) ? "yes" : "no");

    /* madvise 释放建议 */
    madvise(p, len, MADV_DONTNEED);
    printf("madvise MADV_DONTNEED — page content discarded\n");

    munmap(p, len);
    return 0;
}

```

---
