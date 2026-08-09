# Ch56 demos — socket intro (UNIX stream)

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o us_stream us_stream.c

# terminal A
./us_stream server /tmp/ch56.sock

# terminal B
./us_stream client /tmp/ch56.sock "hello-socket"
```

| 文件 | 说明 |
|------|------|
| `us_stream.c` | UNIX `SOCK_STREAM`：bind/listen/accept ↔ connect/write |
