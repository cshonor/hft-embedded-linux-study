# Ch53 demos — POSIX semaphores

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o named_sem_demo named_sem_demo.c -pthread
./named_sem_demo

gcc -Wall -Wextra -o anon_sem_demo anon_sem_demo.c -pthread
./anon_sem_demo
```

| 文件 | 说明 |
|------|------|
| `named_sem_demo.c` | `/name` 二元信号量 wait/post + unlink |
| `anon_sem_demo.c` | `MAP_SHARED` + `sem_init(pshared=1)` 父子握手 |
