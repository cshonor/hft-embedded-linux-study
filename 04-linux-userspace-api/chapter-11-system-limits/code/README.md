# Ch11 demos — System Limits and Options

```bash
cc -Wall -Wextra -o print_limits print_limits.c
./print_limits
./print_limits /tmp /home

# cross-check
getconf ARG_MAX
getconf NAME_MAX /
getconf PAGE_SIZE
```

| 文件 | 说明 |
|------|------|
| `print_limits.c` | 安全 `sysconf`/`pathconf`：`-1` 区分错误 vs indeterminate |
