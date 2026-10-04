# 4.6 stackcount

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.6 节。多用途工具之二：**调用栈计数器**

## 内容详解

`stackcount(8)`：统计**是谁（哪条调用路径）触发了事件**——对每次命中抓取内核/用户态调用栈并按栈聚合计数。

### 书中案例：ktime_get（谁在频繁读时钟）

```bash
stackcount ktime_get
```

输出（节选）：

```
  ktime_get
  __vfs_read
  vfs_read
  sys_read
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  cat
    12
```

→ 立刻看到：`cat` 进程的 `read()` 系统调用路径在调 `ktime_get`，共 12 次。

这个案例的教学价值在于**归因方向的翻转**：普通工具从"进程"往下查（这个进程在干什么），stackcount 从"事件"往上查（谁触发了它）——当事件本身可疑（某函数被调异常频繁）而不知道责任方时，这是唯一方向正确的问法。

### 关键参数

| 参数 | 作用 |
|------|------|
| `-P` | 按 PID 分别统计（同一路径不同进程分开计数） |
| `-f folded` | 输出 **folded 格式**，直接喂给 `flamegraph.pl` 生成火焰图 |
| `-v` | 显示原始地址（符号解析失败时排查用） |
| `-r` | 限定抓内核栈/用户栈/两者（`-r kernel` / `-r user`，版本参数名略有差异） |

```bash
stackcount -f ktime_get > out.folded
flamegraph.pl out.folded > ktime.svg
```

### 内联导致调用栈残缺（真实例子）

栈中 `tick_nohz_start_idle` 一帧"跳变"：函数被编译器**内联**后没有独立的栈帧/符号，回溯时帧缺失或错位。这是第 2 章讲过的 FP/DWARF/LBR 回溯技术共同面对的现实问题——**栈不完整是常态，不是 bug**。

### stackcount 与火焰图的分工

- **文本输出**（默认）：栈数不多时（<20 条路径）直接读，每条路径精确计数
- **folded + 火焰图**：路径发散时（几十上百条）可视化胜出——宽度=占比的直觉来自 2.5 节

判断标准看输出的行数：第一轮跑完看栈条目数，多则 `-f` 重跑画图。注意 stackcount 火焰图是**"触发该事件的栈"分布**，与 profile 采样火焰图（"CPU 时间"分布）语义不同——前者按事件计数，后者按时间占比。

## HFT 关联

- "这个异常 syscall / 这个热点函数**从哪条业务路径**打过来的？" —— stackcount 是标准答案；`-f` + 火焰图适合路径发散的场景。
- 例：`stackcount -P -i 1 't:syscalls:sys_enter_futex'` 定位哪个线程组在疯狂拿锁。
- 交易系统的典型用例：`stackcount 'tcp_drop'`（谁在丢包）、`stackcount -P kfree_skb`（丢包责任路径按进程拆分）、`stackcount 'c:malloc'` 短窗口（分配来源路径，开销大要限时）。
- 与 offcputime 的配合：offcputime 回答"时间丢在哪种等待上"，stackcount 回答"事件是谁触发的"——一个按时间加权、一个按次数计数，先看时间再看次数。

## 陷阱

- ⚠️ 内联函数会造成栈残缺/错位（tick_nohz_start_idle 例）；换 `perf probe` 加显式探针或接受残缺。
- ⚠️ 抓栈本身不便宜（每命中回溯 N 帧）——只比 `trace` 稍便宜，高频事件先 funccount 估量级。
- ⚠️ 用户态栈需要进程带符号且未 strip；容器场景常见"一串十六进制地址"，先用 `-v` 看原始地址再手动 ksym/usym。
- ⚠️ 内核栈与用户栈的拼接处（syscall 边界）常有空隙或重复帧——读图时把边界两帧当作"接口"而不是连续路径。

<details>
<summary>自测题</summary>

1. 怎么把 stackcount 结果变成火焰图？
   <details><summary>答案</summary>`stackcount -f <event> > out.folded` 然后 `flamegraph.pl out.folded > out.svg`。</details>

2. 为什么有的调用栈看起来"跳帧"？
   <details><summary>答案</summary>函数被编译器内联后无独立栈帧，回溯缺失该帧（如 tick_nohz_start_idle 例）。</details>

3. `-P` 参数的作用？
   <details><summary>答案</summary>按 PID 拆分统计，同一路径不同进程分别计数。</details>

4. stackcount 火焰图与 profile 采样火焰图有何语义差异？
   <details><summary>答案</summary>stackcount 按事件次数计数（触发该事件的调用路径分布），profile 按采样时间占比（CPU 时间分布）——一个是"频率视角"，一个是"时间视角"；高频短函数在两图中的占比完全不同。</details>

5. "某内核函数被调异常频繁，想知道责任方"——为什么工具方向是 stackcount 而不是 offcputime？
   <details><summary>答案</summary>问题的主语是"次数"（谁触发的），不是"时间"（等在哪）——stackcount 从事件向上归因（按次数计数），offcputime 按离开 CPU 的时长加权，回答的是另一类问题。</details>
</details>
