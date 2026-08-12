# Ch34 demos — Process groups & sessions

```bash
cc -Wall -Wextra -o print_ids print_ids.c
./print_ids

cc -Wall -Wextra -o setsid_demo setsid_demo.c
./setsid_demo
```

| 文件 | 说明 |
|------|------|
| `print_ids.c` | PID / PGID / SID |
| `setsid_demo.c` | fork 后子进程 `setsid`（组长不能直接 setsid） |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
/* Ch34 demo: setsid */
int main(void) {
    printf("pgrp=%d sid=%d\n", (int)getpgrp(), (int)getsid(0));
    if (fork() == 0) { setsid(); printf("child sid=%d\n", (int)getsid(0)); _exit(0); }
    wait(NULL);
    return 0;
}
```

---
