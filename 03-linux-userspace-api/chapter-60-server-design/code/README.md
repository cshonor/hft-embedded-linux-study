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
