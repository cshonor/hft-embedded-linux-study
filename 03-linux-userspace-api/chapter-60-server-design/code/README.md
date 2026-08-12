# Ch60 demos — server design

```bash
gcc -Wall -Wextra -o fork_sv fork_sv.c
./fork_sv 127.0.0.1 19060

# clients (reuse Ch59 client or nc)
# nc 127.0.0.1 19060
# printf 'hello' | nc 127.0.0.1 19060
```

| 文件 | 说明 |
|------|------|
| `fork_sv.c` | fork 并发 + `SO_REUSEADDR` + `SIGCHLD` 收割 + 正确 close |

## 代码示例

```c

/* 三种服务器模型：迭代 / fork 并发 / 线程并发 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>

#define PORT 9996

/* 迭代服务器：一次只处理一个客户端 */
void iterative_server(void) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(PORT) };
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int opt = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sfd, 5);

    printf("iterative server on port %d\n", PORT);
    /* while (1) { int cfd = accept(...); handle(cfd); close(cfd); } */
    close(sfd);
}

int main(void) {
    iterative_server();
    printf("server models: iterative / fork / thread\n");
    return 0;
}

```

---
