# Ch19 demos — inotify

Linux only（WSL 可用）。

```bash
cc -Wall -Wextra -o inotify_dir inotify_dir.c
mkdir -p /tmp/tlpi_inotify
./inotify_dir /tmp/tlpi_inotify
# other terminal: touch /tmp/tlpi_inotify/a; echo x >> /tmp/tlpi_inotify/a; rm /tmp/tlpi_inotify/a

cc -Wall -Wextra -o inotify_epoll inotify_epoll.c
./inotify_epoll /tmp/tlpi_inotify
# Ctrl-C to quit
```

| 文件 | 说明 |
|------|------|
| `inotify_dir.c` | 阻塞 `read`，打印目录事件（含 move cookie） |
| `inotify_epoll.c` | `IN_NONBLOCK` + `epoll` 事件循环 |

## 代码示例

```c
#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>
/* Ch19 demo: inotify */
int main(void) {
    int fd = inotify_init();
    int wd = inotify_add_watch(fd, "/tmp", IN_CREATE);
    /* 等待事件... */
    char buf[4096];
    int n = read(fd, buf, sizeof(buf));
    /* 处理事件 */
    inotify_rm_watch(fd, wd);
    close(fd);
    return 0;
}
```

---
