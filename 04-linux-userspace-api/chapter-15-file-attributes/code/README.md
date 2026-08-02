# Ch15 demos — File Attributes

```bash
cc -Wall -Wextra -o print_stat print_stat.c
./print_stat /etc/passwd /tmp .
# symlink: ln -s /etc/passwd /tmp/p.link && ./print_stat /tmp/p.link

cc -Wall -Wextra -o umask_demo umask_demo.c
./umask_demo

cc -Wall -Wextra -o futimens_demo futimens_demo.c
./futimens_demo /tmp/tlpi_ts_demo.txt

cc -Wall -Wextra -D_GNU_SOURCE -o statx_btime statx_btime.c
./statx_btime /etc/passwd   # needs Linux 4.11+; btime may be 0 on some FS
```

| 文件 | 说明 |
|------|------|
| `print_stat.c` | `lstat` 打印类型、权限、大小、三时间 |
| `umask_demo.c` | 不同 umask 下创建文件的最终 mode |
| `futimens_demo.c` | 改 atime/mtime，观察 ctime 自动变 |
| `statx_btime.c` | Linux `statx` 尝试打印 birth time |
