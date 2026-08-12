# Ch63 demos — poll / select

```bash
gcc -Wall -Wextra -o poll_stdin poll_stdin.c
# wait up to 5s for a line on stdin
./poll_stdin
# type something and Enter, or wait for timeout
```

| 文件 | 说明 |
|------|------|
| `poll_stdin.c` | `poll(STDIN, POLLIN)` + 超时；对比阻塞 read |

## 代码示例

```c

/* epoll I/O 多路复用：epoll_create1 + epoll_ctl + epoll_wait */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void) {
    int epfd = epoll_create1(0);
    printf("epoll fd: %d\n", epfd);

    /* 添加 stdin 到 epoll */
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    printf("stdin added to epoll (EPOLLIN)\n");

    /* 等待事件（5 秒超时） */
    printf("waiting for input (type something, 5s timeout)...\n");
    struct epoll_event events[4];
    int n = epoll_wait(epfd, events, 4, 5000);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            printf("event on fd=%d, events=0x%x\n",
                   events[i].data.fd, events[i].events);
            if (events[i].data.fd == STDIN_FILENO) {
                char buf[64];
                fgets(buf, sizeof(buf), stdin);
                printf("read: %s", buf);
            }
        }
    } else if (n == 0) {
        printf("epoll_wait timeout\n");
    }

    close(epfd);
    return 0;
}

```

---
