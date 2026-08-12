# Ch25 demos — Process Termination

```bash
cc -Wall -Wextra -o atexit_order atexit_order.c
./atexit_order

cc -Wall -Wextra -o exit_vs_exit exit_vs_exit.c
./exit_vs_exit exit      # flushes "hello" via exit()
./exit_vs_exit _exit    # may lose buffered "hello" when redirected

cc -Wall -Wextra -o fork_atexit fork_atexit.c
./fork_atexit exit      # atexit may run in both (bad)
./fork_atexit _exit     # child skips atexit
```

| 文件 | 说明 |
|------|------|
| `atexit_order.c` | 回调 LIFO |
| `exit_vs_exit.c` | `exit` 刷缓冲 vs `_exit` 不刷 |
| `fork_atexit.c` | fork 后 `exit`/`_exit` 与 atexit |

## 代码示例

```c
#include <stdio.h>
#include <stdlib.h>
/* Ch25 demo: exit vs _exit */
static void cleanup(void) { printf("cleanup\n"); }
int main(void) {
    atexit(cleanup);
    printf("exiting...\n");
    exit(0); /* runs atexit handlers */
}
```

---
