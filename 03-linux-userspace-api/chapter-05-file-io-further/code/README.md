# Ch5 demos

```bash
cc -Wall -Wextra -o dup_share_offset dup_share_offset.c && ./dup_share_offset
cc -Wall -Wextra -o pread_demo pread_demo.c && ./pread_demo
cc -Wall -Wextra -o o_excl_create o_excl_create.c && ./o_excl_create
cc -Wall -Wextra -o fcntl_nonblock fcntl_nonblock.c && ./fcntl_nonblock
```

| 文件 | 演示 |
|------|------|
| `dup_share_offset.c` | `dup` 共享文件偏移 |
| `pread_demo.c` | `pread` 不改当前偏移 |
| `o_excl_create.c` | `O_CREAT\|O_EXCL` 原子创建 |
| `fcntl_nonblock.c` | 管道上 `fcntl` 设 `O_NONBLOCK` |
