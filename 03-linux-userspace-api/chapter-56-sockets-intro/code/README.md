# Ch56 demos — socket intro (UNIX stream)

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o us_stream us_stream.c

# terminal A
./us_stream server /tmp/ch56.sock

# terminal B
./us_stream client /tmp/ch56.sock "hello-socket"
```

| 文件 | 说明 |
|------|------|
| `us_stream.c` | UNIX `SOCK_STREAM`：bind/listen/accept ↔ connect/write |

## 代码示例

```c

/* socket 基本流程：socket → bind → listen → accept → recv/send */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(void) {
    /* 创建 AF_UNIX 流式 socket */
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/ch56_demo.sock", sizeof(addr.sun_path) - 1);
    unlink(addr.sun_path);  /* 清理旧 socket 文件 */

    /* bind */
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }

    /* listen */
    if (listen(sfd, 5) < 0) { perror("listen"); exit(1); }
    printf("listening on %s\n", addr.sun_path);

    /* accept（这里非阻塞模式简化演示） */
    int cfd = accept(sfd, NULL, NULL);
    if (cfd >= 0) {
        char buf[128];
        ssize_t n = recv(cfd, buf, sizeof(buf) - 1, 0);
        if (n > 0) { buf[n] = '\0'; printf("received: %s\n", buf); }
        send(cfd, "ok", 3, 0);
        close(cfd);
    }

    close(sfd);
    unlink(addr.sun_path);
    return 0;
}

```

---
