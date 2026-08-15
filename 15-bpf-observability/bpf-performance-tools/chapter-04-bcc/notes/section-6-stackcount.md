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

### 关键参数

| 参数 | 作用 |
|------|------|
| `-P` | 按 PID 分别统计（同一路径不同进程分开计数） |
| `-f folded` | 输出 **folded 格式**，直接喂给 `flamegraph.pl` 生成火焰图 |
| `-v` | 显示原始地址（符号解析失败时排查用） |

```bash
stackcount -f ktime_get > out.folded
flamegraph.pl out.folded > ktime.svg
```

### 内联导致调用栈残缺（真实例子）

栈中 `tick_nohz_start_idle` 一帧"跳变"：函数被编译器**内联**后没有独立的栈帧/符号，回溯时帧缺失或错位。这是第 2 章讲过的 FP/DWARF/LBR 回溯技术共同面对的现实问题——**栈不完整是常态，不是 bug**。

### 其他

- 事件语法与 funccount 相同（`name` / `lib:name` / `path:name` / `t:` / `u:`）；
- 也支持单行程序与 `-h`。

## HFT 关联

- "这个异常 syscall / 这个热点函数**从哪条业务路径**打过来的？" —— stackcount 是标准答案；`-f` + 火焰图适合路径发散的场景。
- 例：`stackcount -P -i 1 't:syscalls:sys_enter_futex'` 定位哪个线程组在疯狂拿锁。

## 陷阱

- ⚠️ 内联函数会造成栈残缺/错位（tick_nohz_start_idle 例）；换 `perf probe` 加显式探针或接受残缺。
- ⚠️ 抓栈本身不便宜（每命中回溯 N 帧）——只比 `trace` 稍便宜，高频事件先 funccount 估量级。
- ⚠️ 用户态栈需要进程带符号且未 strip；容器场景常见"一串十六进制地址"，先用 `-v` 看原始地址再手动 ksym/usym。

<details>
<summary>自测题</summary>

1. 怎么把 stackcount 结果变成火焰图？
   <details><summary>答案</summary>`stackcount -f <event> > out.folded` 然后 `flamegraph.pl out.folded > out.svg`。</details>

2. 为什么有的调用栈看起来"跳帧"？
   <details><summary>答案</summary>函数被编译器内联后无独立栈帧，回溯缺失该帧（如 tick_nohz_start_idle 例）。</details>

3. `-P` 参数的作用？
   <details><summary>答案</summary>按 PID 拆分统计，同一路径不同进程分别计数。</details>
</details>
