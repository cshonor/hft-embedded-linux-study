# Ch36 demos — Process resources

```bash
cc -Wall -Wextra -o print_rlimit print_rlimit.c
./print_rlimit

cc -Wall -Wextra -o rusage_demo rusage_demo.c
./rusage_demo

cc -Wall -Wextra -o raise_nofile raise_nofile.c
./raise_nofile
```

| 文件 | 说明 |
|------|------|
| `print_rlimit.c` | 打印常见 soft/hard |
| `rusage_demo.c` | SELF + CHILDREN（需 wait） |
| `raise_nofile.c` | 在硬限内抬高 `RLIMIT_NOFILE` |

## 代码示例

```c
#include <stdio.h>
#include <sys/resource.h>
/* Ch36 demo: getrusage */
int main(void) {
    struct rusage u;
    getrusage(RUSAGE_SELF, &u);
    printf("user=%ld.%06ld maxrss=%ldKB\n",
           (long)u.ru_utime.tv_sec, (long)u.ru_utime.tv_usec,
           (long)u.ru_maxrss);
    return 0;
}
```

---
