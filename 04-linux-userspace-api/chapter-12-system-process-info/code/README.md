# Ch12 demos — System and Process Information

```bash
cc -Wall -Wextra -o uname_demo uname_demo.c && ./uname_demo
cc -Wall -Wextra -o proc_self_status proc_self_status.c && ./proc_self_status
cc -Wall -Wextra -o mini_ps mini_ps.c && ./mini_ps | head

# compare
uname -a
cat /proc/self/status | head
ps -o pid,comm
```

| 文件 | 说明 |
|------|------|
| `uname_demo.c` | POSIX `uname` + `gethostname` |
| `proc_self_status.c` | `/proc/self/status` + `cmdline`（`\0`→空格） |
| `mini_ps.c` | 遍历 `/proc/[pid]`，容忍进程消失 |
