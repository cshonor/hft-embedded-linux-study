# Ch34 demos — Process groups & sessions

```bash
cc -Wall -Wextra -o print_ids print_ids.c
./print_ids

cc -Wall -Wextra -o setsid_demo setsid_demo.c
./setsid_demo
```

| 文件 | 说明 |
|------|------|
| `print_ids.c` | PID / PGID / SID |
| `setsid_demo.c` | fork 后子进程 `setsid`（组长不能直接 setsid） |
