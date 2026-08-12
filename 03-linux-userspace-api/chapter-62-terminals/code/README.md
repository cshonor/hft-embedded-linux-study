# Ch62 demos — terminals

须在真实终端运行（管道重定向则 `isatty` 失败）：

```bash
gcc -Wall -Wextra -o noecho_line noecho_line.c
./noecho_line
# type a line (no echo), Enter — attributes restored after
```

| 文件 | 说明 |
|------|------|
| `noecho_line.c` | 关 ECHO 读一行；`atexit` + 信号路径恢复 termios |

## 代码示例

```c

/* termios 终端控制：关闭回显 + 原始模式 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int main(void) {
    struct termios oldtio, newtio;

    /* 保存原始终端设置 */
    tcgetattr(STDIN_FILENO, &oldtio);
    newtio = oldtio;

    /* 关闭回显 */
    newtio.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newtio);
    printf("Echo OFF — type a password (5 chars): ");
    fflush(stdout);

    char buf[16];
    int n = read(STDIN_FILENO, buf, sizeof(buf));
    printf("\nYou typed: %.*s\n", n, buf);

    /* 恢复终端设置 */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
    printf("Echo restored\n");
    return 0;
}

```

---
