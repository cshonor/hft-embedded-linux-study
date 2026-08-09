# Ch61 demos — socket options / names

```bash
gcc -Wall -Wextra -o sockopt_demo sockopt_demo.c
./sockopt_demo
```

| 文件 | 说明 |
|------|------|
| `sockopt_demo.c` | bind 前 `SO_REUSEADDR` + `TCP_NODELAY`；`getsockname` 看临时端口 |
