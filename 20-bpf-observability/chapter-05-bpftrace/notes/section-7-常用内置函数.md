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


### 常见陷阱

1. **str() 用于非字符串参数** — str() 把指针参数当作字符串读取，如果指针指向的不是以 null 结尾的字符串会读到垃圾数据或触发 verifier 拒绝
2. **忽视 printf 的性能影响** — printf 每次调用都把数据送到用户态，高频 probe 中使用会导致严重开销；聚合用 Map，只在低频探针或 END 中用 printf
3. **混淆 kstack 和 ustack 的使用场景** — kstack 获取内核调用栈，ustack 获取用户态调用栈；分析内核路径用 kstack，分析应用逻辑用 ustack，两者可同时使用

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 常用的内置函数有哪些？**

   <details>
   <summary>参考答案</summary>

   str(ptr)：把指针读取为字符串。printf(fmt, args)：格式化打印（低频用）。join(ptr)：打印字符串数组。kstack/ustack：获取内核/用户栈。ntop(ipaddr)：IP 地址转字符串。time(fmt)：打印时间戳。system(cmd)：执行系统命令（慎用，有竞态风险）。

   </details>

2. **str() 函数使用时有什么陷阱？**

   <details>
   <summary>参考答案</summary>

   str() 把指针参数当作 C 字符串读取到用户态，如果：(1) 指针指向非字符串数据（如二进制结构），会读到垃圾；(2) 字符串未以 null 结尾，可能越界读；(3) 指针来自用户空间需要 `str(uptr(arg0))` 而非 `str(arg0)`。verifier 会检查但不是所有情况都能拦截。

   </details>

3. **为什么高频 probe 中不能用 printf？应该用什么替代？**

   <details>
   <summary>参考答案</summary>

   printf 每次调用都通过 ring buffer 把数据送到用户态打印，高频 probe（如每秒万次的 vfs_read）会导致 ring buffer 溢出和 CPU 开销。替代方案：用 Map 聚合（`@[comm] = count()`），在 END 或 interval 中用 `print(@map)` 一次性输出。

   </details>

</details>

---
