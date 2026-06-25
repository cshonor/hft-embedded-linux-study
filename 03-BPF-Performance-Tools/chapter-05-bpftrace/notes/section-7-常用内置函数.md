# 7. 常用内置函数

| 函数 | 作用 |
|------|------|
| `printf(fmt, ...)` | 格式化输出（类似 C） |
| `time(fmt)` | 人类可读时间戳 |
| `join(arr, delim)` | 拼接字符串数组（如 `argv`） |
| `str(ptr)` | 安全读用户/内核内存为字符串 |
| `ksym(addr)` | 内核地址 → 符号名 |
| `usym(addr)` | 用户地址 → 符号名 |
| `kstack` / `ustack` | 栈 ID 或配合 `print(kstack)` |
| `cat(path)` | 读文件内容到字符串（脚本初始化） |
| `system()` | 用户态执行 shell（**慎用**，仅 BEGIN/END） |

```bash
bpftrace -e 'kretprobe:sys_read /@bytes[comm] = sum(retval);/'
bpftrace -e 'tracepoint:syscalls:sys_enter_execve {
    printf("%s %s\n", comm, str(args->filename));
}'
```

---
