# Ch44 demos — pipe & FIFO

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o pipe_demo pipe_demo.c
./pipe_demo

gcc -Wall -Wextra -o fifo_demo fifo_demo.c
# terminal A
./fifo_demo server /tmp/ch44.fifo
# terminal B
./fifo_demo client /tmp/ch44.fifo "hello-fifo"
```

| 文件 | 说明 |
|------|------|
| `pipe_demo.c` | 父写子读；双方关无用端 |
| `fifo_demo.c` | 无亲缘：server 读 / client 写 |
