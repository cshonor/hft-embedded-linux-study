# Ch28 demos — fork/exec deep dive

目录名以仓库为准：`chapter-28-process-creation-exec-detail/`。

```bash
cc -Wall -Wextra -o fork_signal_state fork_signal_state.c
./fork_signal_state

cc -Wall -Wextra -o fork_exec_redirect fork_exec_redirect.c
./fork_exec_redirect /tmp/tlpi_exec_out.txt
cat /tmp/tlpi_exec_out.txt

# CLOEXEC: see ../chapter-27-program-execution/code/cloexec_demo.c
```

| 文件 | 说明 |
|------|------|
| `fork_signal_state.c` | 子进程：掩码继承、pending 清空 |
| `fork_exec_redirect.c` | fork + 重定向 stdout + execvp |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
/* Ch28 demo: fork+exec with FD_CLOEXEC */
int main(void) {
    int fd = open("/dev/null", O_WRONLY);
    printf("fd=%d before fork\n", fd);

    pid_t p = fork();
    if (p == 0) {
        /* 设置 FD_CLOEXEC: exec 后自动关闭 */
        fcntl(fd, F_SETFD, FD_CLOEXEC);
        execlp("ls", "ls", "/proc/self/fd", NULL);
        _exit(1);
    }
    wait(NULL);
    printf("fd=%d still open in parent\n", fd);
    close(fd);
    return 0;
}
```

---
