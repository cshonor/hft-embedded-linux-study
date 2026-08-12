# Ch27 demos — exec

```bash
cc -Wall -Wextra -o fork_exec fork_exec.c
./fork_exec
./fork_exec echo hello from exec

cc -Wall -Wextra -o cloexec_demo cloexec_demo.c
./cloexec_demo
```

| 文件 | 说明 |
|------|------|
| `fork_exec.c` | fork + execvp + waitpid 标准模板 |
| `cloexec_demo.c` | `O_CLOEXEC` 在 exec 后关闭 fd |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
/* Ch27 demo: execlp */
int main(void) {
    if (fork() == 0) { execlp("echo", "echo", "hi", NULL); _exit(1); }
    wait(NULL);
    return 0;
}
```

---
