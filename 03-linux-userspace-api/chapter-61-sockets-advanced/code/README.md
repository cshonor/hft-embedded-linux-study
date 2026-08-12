# Ch61 demos — socket options / names

```bash
gcc -Wall -Wextra -o sockopt_demo sockopt_demo.c
./sockopt_demo
```

| 文件 | 说明 |
|------|------|
| `sockopt_demo.c` | bind 前 `SO_REUSEADDR` + `TCP_NODELAY`；`getsockname` 看临时端口 |

## 代码示例

```c

/* 高级 socket 选项：SO_REUSEADDR / SO_KEEPALIVE / TCP_NODELAY */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    /* SO_REUSEADDR: 允许地址重用 */
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    printf("SO_REUSEADDR: set\n");

    /* SO_KEEPALIVE: TCP 保活 */
    opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    printf("SO_KEEPALIVE: set (detect dead peers)\n");

    /* TCP_NODELAY: 禁用 Nagle 算法（HFT 必用） */
    opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    printf("TCP_NODELAY: set (disable Nagle, low latency)\n");

    /* SO_RCVBUF / SO_SNDBUF: 接收/发送缓冲区大小 */
    int rcvbuf = 0; socklen_t len = sizeof(rcvbuf);
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    printf("SO_RCVBUF: %d bytes\n", rcvbuf);

    int sndbuf = 0; len = sizeof(sndbuf);
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    printf("SO_SNDBUF: %d bytes\n", sndbuf);

    close(fd);
    return 0;
}

```

---
