# Ch50 demos — virtual memory ops

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o vm_ops_demo vm_ops_demo.c
./vm_ops_demo

# if mlock fails: raise lockable memory, e.g.
# ulimit -l unlimited   # or a larger soft limit
```

| 文件 | 说明 |
|------|------|
| `vm_ops_demo.c` | anon mmap → mprotect → mincore → madvise → mlock（可失败） |
