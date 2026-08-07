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


### 常见陷阱

1. **混淆 count() 和 sum()** — count() 每次命中 +1（计数），sum(x) 每次命中加 x 的值（求和）；统计调用次数用 count，统计总字节数用 sum
2. **忽视 hist() 和 lhist() 的区别** — hist() 按 2 的幂自动分桶，lhist() 可指定线性区间和步长；HFT 延迟分布用 hist() 看整体形态，lhist() 看特定区间细节
3. **Map key 设计不当导致膨胀** — 用 pid 或 comm 做 key 时，长时间运行会产生大量条目；应定期清理或用更有针对性的 key

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 的 Map 聚合函数有哪些？各自用途是什么？**

   <details>
   <summary>参考答案</summary>

   count()：每次命中 +1（事件计数）。sum(x)：累加 x 的值（如总字节数）。avg(x)：平均值。min(x)/max(x)：最小/最大值。hist(x)：2 的幂次直方图（延迟分布）。lhist(x,lo,hi,step)：线性直方图（指定区间和步长）。

   </details>

2. **hist() 和 lhist() 有什么区别？HFT 延迟分析该用哪个？**

   <details>
   <summary>参考答案</summary>

   hist() 自动按 2 的幂分桶（1,2,4,8...），适合看整体分布形态。lhist(x,lo,hi,step) 指定线性区间——如 `lhist($lat, 0, 10000, 100)` 看延迟 0-10us 每 100ns 的分布。HFT 延迟分析：先用 hist() 看整体形态定位问题区间，再用 lhist() 对异常区间做精细分析。

   </details>

3. **如何避免 Map 因 key 过多而膨胀？**

   <details>
   <summary>参考答案</summary>

   (1) 用 interval 定期 `clear(@map)` 清空旧数据；(2) 用更有针对性的 key（如按 comm 而非 pid，减少条目数）；(3) 用 `delete(@map[key])` 在用完后清理（如 retprobe 中删除 start[tid]）；(4) 限制脚本运行时长。

   </details>

</details>

---
