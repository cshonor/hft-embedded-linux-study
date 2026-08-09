# Ch26 demos — wait / waitpid

```bash
cc -Wall -Wextra -o waitpid_status waitpid_status.c
./waitpid_status
./waitpid_status signal    # child raises SIGTERM

cc -Wall -Wextra -o sigchld_reap_loop sigchld_reap_loop.c
./sigchld_reap_loop
```

| 文件 | 说明 |
|------|------|
| `waitpid_status.c` | 解析正常退出 / 信号杀死 |
| `sigchld_reap_loop.c` | SIGCHLD + while WNOHANG 收割多子 |
