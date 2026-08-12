# Ch22 demos — Advanced signals

```bash
cc -Wall -Wextra -o sigsuspend_wait sigsuspend_wait.c
./sigsuspend_wait
# press Ctrl+C

cc -Wall -Wextra -o sigwaitinfo_loop sigwaitinfo_loop.c
./sigwaitinfo_loop
# other shell: kill -USR1 <pid> ; kill -USR2 <pid>

cc -Wall -Wextra -o sigqueue_rt sigqueue_rt.c
./sigqueue_rt
# prints queued realtime signal with int payload (self-send)
```

| 文件 | 说明 |
|------|------|
| `sigsuspend_wait.c` | 阻塞 SIGINT + `sigsuspend` 安全等待 |
| `sigwaitinfo_loop.c` | 同步取出 pending（无 handler） |
| `sigqueue_rt.c` | `sigqueue` 传 int + `SA_SIGINFO` |

## 代码示例

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
/* Ch22 demo: sigsuspend */
void h(int s) { printf("caught %d\n", s); }
int main(void) {
    signal(SIGINT, h);
    sigset_t m; sigemptyset(&m); sigaddset(&m, SIGINT);
    sigprocmask(SIG_BLOCK, &m, NULL);
    printf("blocked, waiting...\n");
    sigsuspend(&m); /* 原子解除+等待 */
    return 0;
}
```

---
