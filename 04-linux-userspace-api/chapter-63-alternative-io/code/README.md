# Ch63 demos — poll / select

```bash
gcc -Wall -Wextra -o poll_stdin poll_stdin.c
# wait up to 5s for a line on stdin
./poll_stdin
# type something and Enter, or wait for timeout
```

| 文件 | 说明 |
|------|------|
| `poll_stdin.c` | `poll(STDIN, POLLIN)` + 超时；对比阻塞 read |
