# Ch64 demos — POSIX PTY

在 Linux / WSL 下：

```bash
gcc -Wall -Wextra -o pty_shell pty_shell.c
./pty_shell
# prints slave path + output of `tty` and echo via shell on the PTY
```

| 文件 | 说明 |
|------|------|
| `pty_shell.c` | openpt 流程 + fork/setsid/dup2 + 读 master |
