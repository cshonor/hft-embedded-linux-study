## 15.2 bpftrace

> 章节导航：[15.1.7 BCC vs bpftrace](./section-15.1.7-BCC-vs-bpftrace.md) · 上一篇 ← · [本章导读](../README.md)

**本节讲什么**：bpftrace 的语言模型（probe / filter / action 三段式）、探针类型速查、map 与内置变量、单行命令的写法套路、生产纪律。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 语言模型 = **probe /filter /action** | 和 awk 的 pattern-action 同构 |
| 2 | **聚合在内核**，print 在用户态 | 高频事件永远用 map |
| 3 | 单行是它的**统治区间** | 10× 于写 BCC Python |
| 4 | **-l 列探针，先确认再挂** | 挂错探针 = 白跑 |
| 5 | 生产前 staging 验证**开销** | 观测者效应是真实的 |

---

### 一、语言模型：三段式

```
probe[,probe...]  /filter/  { action; action; }
 └─ 什么时候              └─ 附加条件     └─ 干什么
```

```bash
# 统计 read syscall 调用次数
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_read { @ = count(); }'

# 只统计 strategy 进程的 openat
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat /pid == 12345/ { @[comm] = count(); }'

# 多探针共享一个 action（大括号语法）
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_{read,write} /pid==12345/ { @[probe] = count(); }'
```

### 二、探针类型速查

| 探针 | 语法 | 场景 | 开销 |
|------|------|------|------|
| **tracepoint** | `tracepoint:syscalls:sys_enter_read` | 稳定 API，首选 | 低 |
| **kprobe** | `kprobe:vfs_read` | 内核函数入口（无 tracepoint 时） | 中 |
| **kretprobe** | `kretprobe:vfs_read` | 函数返回（retval） | 中 |
| **uprobe** | `uprobe:/opt/bin/strategy:decode` | 用户函数入口 | 中 |
| **uretprobe** | `uretprobe:/opt/bin/strategy:decode` | 用户函数返回 | 中 |
| **USDT** | `usdt:/opt/bin/strategy:probe_name` | 应用埋点（稳定，推荐） | 低 |
| **profile** | `profile:hz:99` | 定频采样（on-CPU） | 低 |
| **interval** | `interval:s:1` | 每秒一次（输出心跳） | 极低 |
| **software** | `software:cpu-clock:99` | 软件 PMC（对齐 perf） | 低 |
| **hardware** | `hardware:cache-misses:99` | 硬件 PMC | 低 |

选择顺序：**有 tracepoint/USDT 绝不用 kprobe/uprobe**——前者是稳定 ABI，内核/应用升级不破；后者函数名一变就失效。

```bash
# 先列出可用探针，确认名字再挂（避免拼写错误白跑）
sudo bpftrace -l 'tracepoint:syscalls:*' | head
sudo bpftrace -l 'kprobe:*mutex*' | head
```

### 三、map：内核态聚合

| map | 语义 | 典型用法 |
|-----|------|---------|
| `@ = count()` | 计数器 | 事件频率 |
| `@ = hist(x)` | **对数直方图** | 延迟分布（最有价值） |
| `@[k] = count()` | 按 k 分组计数 | 按 pid/comm 聚合 |
| `@[k] = hist(x)` | 按 k 分组的直方图 | 每进程延迟分布 |
| `@s[tid] = nsecs` | 时间戳暂存 | 出入对（测区间） |
| `lhist(x, min, max, step)` | 线性直方图 | 已知量程（如队列深度） |
| `@ = sum/min/max/avg(x)` | 聚合函数 | 统计摘要 |

**出入对模式**（测函数耗时的标准骨架）：

```bash
sudo bpftrace -e '
  uprobe:/opt/strategy:decode { @start[tid] = nsecs; }
  uretprobe:/opt/strategy:decode /@start[tid]/ {
      @decode_us = hist((nsecs - @start[tid]) / 1000);
      delete(@start[tid]);
  }'
```

三个细节：
1. `/@start[tid]/` 过滤掉没有入记录的返回（重入/跳过路径）；
2. `delete()` 防止 map 泄漏（tid 会复用）；
3. **内核里只存时间戳和分桶**，直方图渲染在用户态 Ctrl-C 时完成——这就是高频不打爆的秘密（与 [ch14 hist trigger](../../chapter-14-ftrace/notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)、[BCC BPF_HISTOGRAM](./section-15.1-BCC-BPF-Compiler-Collection.md) 同一思路，三家都靠内核聚合）。

### 四、内置变量速查

| 变量 | 含义 |
|------|------|
| `pid` / `tid` | 进程/线程 ID |
| `comm` | 进程名（16 字节） |
| `nsecs` | 纳秒时间戳（单调钟） |
| `cpu` | CPU ID |
| `curtask` | 当前 task_struct（可探 struct 成员） |
| `args` | tracepoint 参数（`args->filename`，稳定） |
| `retval` | kretprobe/uretprobe 返回值 |
| `func` | 探针函数名 |
| `ustack` / `kstack` | 用户/内核栈 |

### 五、单行命令套路库

```bash
# 新进程追踪（execve 逐次）
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'

# 每秒输出一次 CPU 分布心跳
sudo bpftrace -e 'interval:s:1 { printf("tick\n"); }'

# 按进程统计块 I/O 大小分布
sudo bpftrace -e 'tracepoint:block:block_rq_issue { @bytes[comm] = hist(args->bytes); }'

# 内核函数调用频率 top
sudo bpftrace -e 'kprobe:vfs_* { @[func] = count(); }'   # Ctrl-C 后看排行

# 定频采样内核栈（自制 profiler 雏形）
sudo bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'
```

完整弹药库：[附录 C bpftrace 单行命令](../../appendix-C-bpftrace单行命令.md)。

### 六、生产纪律

| 纪律 | 原因 |
|------|------|
| **先 -l 确认探针存在** | 拼错名字静默失败或挂错点 |
| **staging 先跑，量开销** | kprobe 挂在超热路径（每秒百万次）会有可测开销 |
| **永远用 map 聚合，别 printf 每事件** | sched_switch 级别的事件 printf 会打爆 CPU |
| **限 pid/限时长** | `/pid==X/` filter 在内核执行，代价最小 |
| **`--unsafe` 系列（如 kaddr）慎用** | 绕过验证器保护的接口 |
| **exit 用 END 块输出** | `END { print(@); clear(@); }` 收尾渲染 |

### HFT / 嵌入式关联

- **热路径函数延迟**：uprobe/uretprobe 出入对是「策略 decode 耗时分布」的最快答案——不用改一行代码，比加日志干净（无锁、无内存分配）。
- **发现未知 syscall**：`sys_enter_*` 通配 + 按 probe 分组计数，一次看清热路径进程到底碰了哪些 syscall（与 [perf trace -s](../../chapter-13-perf/notes/section-13.11-perf-trace-系统调用追踪.md) 同结论交叉验证）。
- **Pi5 实操**：本节所有单行都可在 ebpf-gate 仓库（cshonor/ebpf-gate，独立 repo）的 bpftrace lab 里跑——06.7 与实验线在那里合流。
- **代价意识**：uprobe 会给目标函数加 ~1-2µs 级别开销（陷阱 + 上下文切换语义）——测纳秒级路径时这个观测者效应不可忽略，改用 tracepoint 或预埋 USDT。

### 衔接

- 上一节：[15.1.7 BCC vs bpftrace](./section-15.1.7-BCC-vs-bpftrace.md)（什么时候用它、什么时候不用）
- 深入：[06.7-BPF ch05 bpftrace](../../../06.7-bpf-observability/02-bpf-performance-tools/chapter-05-bpftrace/)（语言参考全书级展开）
- 弹药库：[附录 C bpftrace 单行命令](../../appendix-C-bpftrace单行命令.md)

---

### 常见陷阱

1. **bpftrace 生产直接跑自定义脚本**——自定义 kprobe 可能加载失败或开销过大，应先 staging 验证。
2. **bpftrace 每事件输出**——高频事件（sched_switch）每条 print 打爆 CPU，应用 map 聚合。
3. **bpftrace 当 BCC 用**——复杂状态机/多 map 协作用 BCC Python，bpftrace DSL 表达力有限。
4. **有 tracepoint 偏用 kprobe**——tracepoint 是稳定 ABI，kprobe 挂函数名，内核升级即失效。
5. **出入对忘了 delete(@start[tid])**——tid 复用会串数据，map 也会缓慢增长。

<details>
<summary>自测题（点击展开）</summary>

1. bpftrace 自定义脚本在生产环境的风险？
   <details><summary>答</summary>kprobe 可能加载失败/开销过大——应先在 staging 验证加载和开销。</details>
2. 为什么 bpftrace 高频事件不能每条 print？
   <details><summary>答</summary>sched_switch 每秒上千次——每条送到用户态会打爆 CPU，应用 map histogram 在内核聚合。</details>
3. bpftrace 的三段式语言模型？
   <details><summary>答</summary>probe（什么时候）+ /filter/（附加条件，内核态执行）+ { action }（做什么）——与 awk 的 pattern-action 同构。</details>
4. 测一个用户函数的耗时分布，探针和 map 怎么选？
   <details><summary>答</summary>uprobe 存 @start[tid]=nsecs，uretprobe 里 /@start[tid]/ 过滤后 hist((nsecs-@start[tid])/1000)，记得 delete。</details>
5. kprobe 与 tracepoint 的选择顺序？
   <details><summary>答</summary>有 tracepoint 绝不用 kprobe：tracepoint 参数 ABI 稳定（args->），kprobe 绑函数名，内核升级即失效。</details>

</details>


---

← [本章导读](../README.md)
