# Ch57 demos — UNIX domain sockets

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o socketpair_demo socketpair_demo.c
./socketpair_demo

gcc -Wall -Wextra -o abstract_stream abstract_stream.c
# terminal A
./abstract_stream server
# terminal B
./abstract_stream client "hello-abstract"

gcc -Wall -Wextra -o uds_dgram uds_dgram.c
# terminal A
./uds_dgram server /tmp/ch57-dgram.sock
# terminal B
./uds_dgram client /tmp/ch57-dgram.sock "msg-1"
```

路径型 STREAM 见 Ch56 `us_stream`。

| 文件 | 说明 |
|------|------|
| `socketpair_demo.c` | 父子双向 STREAM |
| `abstract_stream.c` | Linux 抽象名 STREAM C/S |
| `uds_dgram.c` | 路径型 DGRAM（两端 bind） |
