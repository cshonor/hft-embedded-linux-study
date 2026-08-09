# Ch51 demos — POSIX IPC naming / lifecycle

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o posix_name_demo posix_name_demo.c -lrt
./posix_name_demo
# inspect: ls -l /dev/shm
```

| 文件 | 说明 |
|------|------|
| `posix_name_demo.c` | `shm_open` → close → unlink（导论生命周期） |
