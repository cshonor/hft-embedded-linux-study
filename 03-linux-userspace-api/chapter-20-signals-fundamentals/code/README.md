# Ch20 demos — Signals fundamentals

```bash
cc -Wall -Wextra -o block_pending block_pending.c
./block_pending
# while blocked: press Ctrl+C (may queue as pending), then wait for unblock

cc -Wall -Wextra -o kill_probe kill_probe.c
./kill_probe $$
./kill_probe 1
./kill_probe 999999
```

| 文件 | 说明 |
|------|------|
| `block_pending.c` | 阻塞 SIGINT → sigpending → 解除并递送 |
| `kill_probe.c` | `kill(pid, 0)` 探测进程是否存在 |

## 代码示例

```c
#include <stdio.h>
#include <signal.h>
/* Ch20 demo: signal */
void h(int s) { printf("signal %d\n", s); }
int main(void) {
    signal(SIGUSR1, h);
    raise(SIGUSR1);
    return 0;
}
```

---
