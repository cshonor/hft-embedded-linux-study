# Ch57 demos — UNIX domain sockets

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o socketpair_demo socketpair_demo.c
./socketpair_demo

gcc -Wall -Wextra -o abstract_stream abstract_stream.c
# terminal A
./abstract_stream server
# terminal B
./abstract_stream client "hello-abstract"

gcc -Wall -Wextra -o uds_dgram uds_dgram.c
# terminal A
./uds_dgram server /tmp/ch57-dgram.sock
# terminal B
./uds_dgram client /tmp/ch57-dgram.sock "msg-1"
```

路径型 STREAM 见 Ch56 `us_stream`。

| 文件 | 说明 |
|------|------|
| `socketpair_demo.c` | 父子双向 STREAM |
| `abstract_stream.c` | Linux 抽象名 STREAM C/S |
| `uds_dgram.c` | 路径型 DGRAM（两端 bind） |

## 代码示例

```c

/* AF_UNIX 流式 + 数据报 socket 对比 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(void) {
    /* 流式 socket (类似 TCP) */
    int s1 = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un a1 = { .sun_family = AF_UNIX };
    strncpy(a1.sun_path, "/tmp/ch57_stream.sock", sizeof(a1.sun_path) - 1);
    unlink(a1.sun_path);
    bind(s1, (struct sockaddr *)&a1, sizeof(a1));
    listen(s1, 1);
    printf("stream socket: %s (reliable, ordered)\n", a1.sun_path);

    /* 数据报 socket (类似 UDP) */
    int s2 = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un a2 = { .sun_family = AF_UNIX };
    strncpy(a2.sun_path, "/tmp/ch57_dgram.sock", sizeof(a2.sun_path) - 1);
    unlink(a2.sun_path);
    bind(s2, (struct sockaddr *)&a2, sizeof(a2));
    printf("dgram socket:  %s (unreliable, unordered)\n", a2.sun_path);

    /* 抽象命名空间：sun_path[0] = '\0' */
    int s3 = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un a3 = { .sun_family = AF_UNIX };
    a3.sun_path[0] = '\0';
    strncpy(a3.sun_path + 1, "abstract_name", sizeof(a3.sun_path) - 1);
    bind(s3, (struct sockaddr *)&a3, sizeof(struct sockaddr_un));
    printf("abstract socket: \\0abstract_name (no filesystem entry)\n");

    close(s1); close(s2); close(s3);
    unlink(a1.sun_path);
    unlink(a2.sun_path);
    return 0;
}

```

---
