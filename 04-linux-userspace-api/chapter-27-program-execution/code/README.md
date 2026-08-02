# Ch27 demos — exec

```bash
cc -Wall -Wextra -o fork_exec fork_exec.c
./fork_exec
./fork_exec echo hello from exec

cc -Wall -Wextra -o cloexec_demo cloexec_demo.c
./cloexec_demo
```

| 文件 | 说明 |
|------|------|
| `fork_exec.c` | fork + execvp + waitpid 标准模板 |
| `cloexec_demo.c` | `O_CLOEXEC` 在 exec 后关闭 fd |
