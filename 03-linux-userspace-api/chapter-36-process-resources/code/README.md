# Ch36 demos — Process resources

```bash
cc -Wall -Wextra -o print_rlimit print_rlimit.c
./print_rlimit

cc -Wall -Wextra -o rusage_demo rusage_demo.c
./rusage_demo

cc -Wall -Wextra -o raise_nofile raise_nofile.c
./raise_nofile
```

| 文件 | 说明 |
|------|------|
| `print_rlimit.c` | 打印常见 soft/hard |
| `rusage_demo.c` | SELF + CHILDREN（需 wait） |
| `raise_nofile.c` | 在硬限内抬高 `RLIMIT_NOFILE` |
