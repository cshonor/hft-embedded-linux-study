# Ch21 demos — Signal handlers

```bash
cc -Wall -Wextra -o flag_handler flag_handler.c
./flag_handler
# press Ctrl+C a few times, then wait for exit

cc -Wall -Wextra -o eintr_read eintr_read.c
./eintr_read
# press Ctrl+C while blocked on read — see EINTR (no SA_RESTART)

cc -Wall -Wextra -o siginfo_demo siginfo_demo.c
./siginfo_demo
# from another shell: kill -USR1 <pid>

cc -Wall -Wextra -o sigchld_reap sigchld_reap.c
./sigchld_reap
```

| 文件 | 说明 |
|------|------|
| `flag_handler.c` | `sig_atomic_t` 范式 + `sa_mask` |
| `eintr_read.c` | 无 `SA_RESTART` 时 `read` → `EINTR` |
| `siginfo_demo.c` | `SA_SIGINFO` 打印发送者 |
| `sigchld_reap.c` | `SIGCHLD` 循环 `waitpid(WNOHANG)` |

## 代码示例

```c
#include <stdio.h>
#include <signal.h>
/* Ch21 demo: sigaction */
void h(int s, siginfo_t *i, void *c) { printf("got %d\n", s); }
int main(void) {
    struct sigaction sa = {0};
    sa.sa_sigaction = h;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
    raise(SIGUSR1);
    return 0;
}
```

---
