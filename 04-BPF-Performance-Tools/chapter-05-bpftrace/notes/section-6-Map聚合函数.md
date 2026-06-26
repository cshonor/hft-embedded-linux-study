# 6. Map 聚合函数

海量事件 **在内核完成统计**，不把每条记录送到用户态：

| 函数 | 作用 |
|------|------|
| `count()` | 事件次数 |
| `sum(expr)` | 求和 |
| `avg(expr)` | 平均值 |
| `min()` / `max()` | 极值 |
| `stats(expr)` | count + sum + avg + min + max |
| `hist(expr)` | **2 的幂次方** 直方图（延迟分布首选） |
| `lhist(expr, min, max, step)` | **线性** 直方图 |

```bash
# 读延迟分布（enter/exit 配对示意）
bpftrace -e '
kprobe:vfs_read
/@start[tid]/
{
    @us = hist(nsecs - @start[tid]);
    delete(@start[tid]);
}
kprobe:vfs_read
{
    @start[tid] = nsecs;
}
'

# 按进程统计 syscall 次数
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[comm] = count(); }'
```

**HFT：** 延迟问题优先 `hist()` / `lhist()` — 与 [Ch 3](../../chapter-03-performance-analysis/)「直方图优于均值」一致；勿对 `send`/`recv` 每包 `printf`。

---
