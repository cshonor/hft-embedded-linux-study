# Ch44 demos — pipe & FIFO

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o pipe_demo pipe_demo.c
./pipe_demo

gcc -Wall -Wextra -o fifo_demo fifo_demo.c
# terminal A
./fifo_demo server /tmp/ch44.fifo
# terminal B
./fifo_demo client /tmp/ch44.fifo "hello-fifo"
```

| 文件 | 说明 |
|------|------|
| `pipe_demo.c` | 父写子读；双方关无用端 |
| `fifo_demo.c` | 无亲缘：server 读 / client 写 |

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
/* Ch44 demo: pipe + fork */
int main(void) {
    int fd[2]; pipe(fd);
    if (fork() == 0) {
        close(fd[0]); write(fd[1], "hi", 2); _exit(0);
    }
    close(fd[1]); char b[4]; read(fd[0], b, 2);
    printf("got: %s\n", b); wait(NULL);
    return 0;
}
```

---
