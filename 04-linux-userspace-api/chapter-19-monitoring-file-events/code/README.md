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
