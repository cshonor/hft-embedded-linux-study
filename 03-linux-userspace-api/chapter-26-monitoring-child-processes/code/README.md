# Ch26 demos — wait / waitpid

```bash
cc -Wall -Wextra -o waitpid_status waitpid_status.c
./waitpid_status
./waitpid_status signal    # child raises SIGTERM

cc -Wall -Wextra -o sigchld_reap_loop sigchld_reap_loop.c
./sigchld_reap_loop
```

| 文件 | 说明 |
|------|------|
| `waitpid_status.c` | 解析正常退出 / 信号杀死 |
| `sigchld_reap_loop.c` | SIGCHLD + while WNOHANG 收割多子 |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
/* Ch26 demo: waitpid */
int main(void) {
    pid_t p = fork();
    if (p == 0) _exit(42);
    int s; waitpid(p, &s, 0);
    printf("exit code: %d\n", WEXITSTATUS(s));
    return 0;
}
```

---
