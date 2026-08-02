# Ch54 demos — POSIX shared memory

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o posix_shm_demo posix_shm_demo.c -lrt
./posix_shm_demo

# leftovers: ls /dev/shm ; rm /dev/shm/ch54_demo
```

| 文件 | 说明 |
|------|------|
| `posix_shm_demo.c` | shm_open → ftruncate → mmap → 父写子读 → unlink |
