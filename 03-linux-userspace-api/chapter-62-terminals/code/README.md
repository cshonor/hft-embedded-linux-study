# Ch62 demos — terminals

须在真实终端运行（管道重定向则 `isatty` 失败）：

```bash
gcc -Wall -Wextra -o noecho_line noecho_line.c
./noecho_line
# type a line (no echo), Enter — attributes restored after
```

| 文件 | 说明 |
|------|------|
| `noecho_line.c` | 关 ECHO 读一行；`atexit` + 信号路径恢复 termios |
