# Ch59 demos — Internet domain sockets

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o tcp_iter_sv tcp_iter_sv.c
gcc -Wall -Wextra -o tcp_iter_cl tcp_iter_cl.c
gcc -Wall -Wextra -o udp_echo_sv udp_echo_sv.c
gcc -Wall -Wextra -o udp_echo_cl udp_echo_cl.c

# TCP (loopback)
./tcp_iter_sv 127.0.0.1 19059    # terminal A
./tcp_iter_cl 127.0.0.1 19059 hi # terminal B

# UDP
./udp_echo_sv 127.0.0.1 19060
./udp_echo_cl 127.0.0.1 19060 ping
```

全部用 `getaddrinfo`；服务端 `AI_PASSIVE` 风格（显式地址亦可）。

| 文件 | 说明 |
|------|------|
| `tcp_iter_*.c` | 迭代 TCP：一行请求/应答；忽略 SIGPIPE |
| `udp_echo_*.c` | UDP sendto/recvfrom 回显 |

## 代码示例

```c

/* AF_INET TCP 服务器 + 客户端 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

int main(void) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(9999) };
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sfd, 5);
    printf("server: listening on 0.0.0.0:9999\n");

    pid_t pid = fork();
    if (pid == 0) {
        int cfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in saddr = { .sin_family = AF_INET, .sin_port = htons(9999) };
        inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr);
        connect(cfd, (struct sockaddr *)&saddr, sizeof(saddr));
        send(cfd, "hello TCP", 10, 0);
        char buf[64];
        ssize_t n = recv(cfd, buf, sizeof(buf)-1, 0);
        if (n > 0) { buf[n]='\0'; printf("client: reply='%s'\n", buf); }
        close(cfd);
        _exit(0);
    }

    int afd = accept(sfd, NULL, NULL);
    char buf[64];
    ssize_t n = recv(afd, buf, sizeof(buf)-1, 0);
    if (n > 0) { buf[n]='\0'; printf("server: received='%s'\n", buf); }
    send(afd, "world", 6, 0);
    close(afd);

    wait(NULL);
    close(sfd);
    return 0;
}

```

---
