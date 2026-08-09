# Ch55 demos — file locking

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o pidfile_lock pidfile_lock.c
# terminal A
./pidfile_lock /tmp/ch55.pid
# terminal B (should fail while A holds lock)
./pidfile_lock /tmp/ch55.pid

gcc -Wall -Wextra -o flock_demo flock_demo.c
./flock_demo /tmp/ch55.flock
```

| 文件 | 说明 |
|------|------|
| `pidfile_lock.c` | `fcntl` `F_WRLCK` 单实例 pid 文件 |
| `flock_demo.c` | `LOCK_EX\|LOCK_NB` 整文件锁 |
