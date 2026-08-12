# Ch64 demos — POSIX PTY

在 Linux / WSL 下：

```bash
gcc -Wall -Wextra -o pty_shell pty_shell.c
./pty_shell
# prints slave path + output of `tty` and echo via shell on the PTY
```

| 文件 | 说明 |
|------|------|
| `pty_shell.c` | openpt 流程 + fork/setsid/dup2 + 读 master |

## 代码示例

```c

/* 伪终端 (PTY) 基本用法：posix_openpt + grantpt + unlockpt */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    /* 打开 PTY master */
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) { perror("posix_openpt"); exit(1); }

    /* grantpt: 设置 slave 的权限 */
    if (grantpt(master) < 0) { perror("grantpt"); exit(1); }

    /* unlockpt: 解锁 slave */
    if (unlockpt(master) < 0) { perror("unlockpt"); exit(1); }

    /* ptsname: 获取 slave 设备名 */
    char *slave_name = ptsname(master);
    printf("PTY master fd: %d\n", master);
    printf("PTY slave name: %s\n", slave_name);

    /* 打开 slave */
    int slave = open(slave_name, O_RDWR);
    if (slave < 0) { perror("open slave"); exit(1); }
    printf("PTY slave fd: %d\n", slave);

    /* master 写，slave 读 */
    write(master, "hello pty\n", 10);
    char buf[64];
    int n = read(slave, buf, sizeof(buf) - 1);
    if (n > 0) { buf[n] = '\0'; printf("slave read: %s", buf); }

    close(slave);
    close(master);
    return 0;
}

```

---
