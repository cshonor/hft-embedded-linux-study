# Ch24 demos — fork()

```bash
cc -Wall -Wextra -o fork_basic fork_basic.c
./fork_basic

cc -Wall -Wextra -o fork_stdio_buf fork_stdio_buf.c
./fork_stdio_buf          # may duplicate "before fork" without newline
./fork_stdio_buf fflush   # fixed

cc -Wall -Wextra -o fork_fd_offset fork_fd_offset.c
./fork_fd_offset /tmp/tlpi_fork_fd.txt
cat /tmp/tlpi_fork_fd.txt
```

| 文件 | 说明 |
|------|------|
| `fork_basic.c` | 返回值、PID、COW 全局变量 |
| `fork_stdio_buf.c` | 缓冲重复输出 / fflush |
| `fork_fd_offset.c` | 父子共享文件偏移 |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
/* Ch24 demo: fork */
int main(void) {
    pid_t p = fork();
    if (p == 0) { printf("child\n"); _exit(0); }
    wait(NULL);
    printf("parent\n");
    return 0;
}
```

---
