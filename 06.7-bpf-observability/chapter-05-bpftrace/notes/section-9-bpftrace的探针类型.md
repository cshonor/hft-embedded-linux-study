# 5.9 bpftrace 的探针类型

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.9 节（印刷 p157–162）

## 内容详解

### 表 5-2：探针类型总表（含缩写）

| 类型 | 缩写 | 描述 |
|------|------|------|
| tracepoint | t | 内核静态插桩点 |
| usdt | U | 用户态静态定义插桩点 |
| kprobe | k | 内核动态函数插桩 |
| kretprobe | kr | 内核动态函数返回值插桩 |
| uprobe | u | 用户态动态函数插桩 |
| uretprobe | ur | 用户态动态函数返回值插桩 |
| software | | 内核软件事件 |
| hardware | | 硬件基于计数器的插桩 |
| profile | | 对全部 CPU 进行时间采样 |
| interval | | 周期性报告（从一个 CPU 上） |
| BEGIN / END | | bpftrace 启动 / 退出 |

这些类型是对第 2 章内核技术（kprobes/uprobes/tracepoints/USDT/PMC）的接口。注意有些探针**触发频率很高**（调度、内存分配、网络收发包）——尽量用低频事件（第 18 章讲减少开销）。

### 5.9.1 tracepoint

```
tracepoint:tracepoint_name    # 全名含类别冒号，如 tracepoint:net:netif_rx
```

- 参数经内置结构体变量 **`args`** 访问：`net:netif_rx` 的包长 `args->len`；
- **新手最佳练习对象是 syscall 跟踪点**：覆盖内核资源使用、API 文档全（read(2) man）：

```bash
# bpftrace -l -v 'tracepoint:syscalls:sys_enter_read'   # 查看参数定义
tracepoint:syscalls:sys_enter_read
    int syscall_nr;
    unsigned int fd;
    char * buf;
    size_t count;
```

入口参数 `args->fd/buf/count`；出口跟踪点 `sys_exit_read` 的 `args->ret` 即返回值（另有 man 未列的 `syscall_nr`）。

**经典案例：clone(2) 进入 1 次返回 2 次**——父进程进入 clone，父进程返回子 PID（27804），子进程返回 0。且子进程起初 comm 仍是 "bash"（未 exec），`t:syscalls:sys_*execve` 可见进入时 "bash"、退出时已变 "ls"。

### 5.9.2 usdt

```
usdt:binary_path:probe_name
usdt:library_path:probe_name
usdt:binary_path:probe_namespace:probe_name   # 多命名空间时（如 JVM 的 hotspot）
```

- 例：MySQL `usdt:/usr/local/sbin/mysqld:query_start`；JVM `usdt:/../libjvm.so:hotspot:method_entry`；
- 未指定命名空间时默认与二进制/库名相同；
- 参数同样经 `args->` 访问；
- `bpftrace -l 'usdt:/usr/local/cpython/python'` 列出二进制全部探针（line、function__entry、gc__start…）；`-p PID` 列**运行中进程**的 USDT 探针。

### 5.9.3 kprobe 和 kretprobe

```
kprobe:vfs_read      # 入口
kretprobe:vfs_read   # 返回
```

- **kprobe 参数**：`arg0, arg1, ... argN`（进入函数时的参数，恒为 **64 位无符号整型**）；指向结构体的指针可**强制类型转换**（未来 BTF 自动化该过程）；
- **kretprobe 参数**：内置 `retval`（返回值，恒 64 位无符号，必要时类型转换）。

### 5.9.4 uprobe 和 uretprobe

```
uprobe:binary_path:function_name     # 例：uprobe:/bin/bash:readline
uretprobe:library_path:function_name
```

参数规则与 kprobe/kretprobe 相同（argN 恒 u64、retval 恒 u64）。

### 5.9.5 software 和 hardware

```
software:event_name:count     # 每 count 次事件触发一次
hardware:event_name:count
```

- 软件事件类似跟踪点，但适合**基于计数器的指标与采样探测**；硬件事件是 PMC 子集；
- **count 采样**是关键：这两类事件高频，逐事件插桩开销显著，`software:page-faults:100` = 每 100 次缺页触发一次。

表 5-3 软件事件（默认采样间隔）：`cpu-clock`(1M)、`task-clock`(1M)、`faults/page-faults`(100)、`context-switches`(1K)、`cpu-migrations`、`minor-faults`(100)、`major-faults`、`alignment-faults`、`emulation-faults`、`dummy`、`bpf-output`。

表 5-4 硬件事件（默认采样间隔更大）：`cycles`(1M)、`instructions`(1M)、`cache-references`(1M)、`cache-misses`(1M)、`branch-instructions`(100K)、`bus-cycles`(100K)、`frontend-stalls`(1M)、`backend-stalls`(1M)、`ref-cycles`(1M)。

### 5.9.6 profile 和 interval

```
profile:hz:99    # 全部 CPU 上每秒 99 次（采样 CPU 使用）
interval:s:1     # 单 CPU 上每秒 1 次（周期打印）
```

rate 单位：`hz / s / ms / us`。**为什么 99Hz 而不是 100Hz**——避免与事件周期对齐造成**锁定步进（lockstep）采样**偏差。

## HFT 关联

- CPU 热点采样 `profile:hz:99` + kstack → 自制 on-CPU 火焰图；`interval:s:1 { print(@); clear(@); }` → 每秒滚动指标；
- syscall 跟踪点参数稳定（tracepoint 是稳定 ABI），生产脚本优先 `t:syscalls:*` 而非 kprobe（内核函数随版本变）。

## 陷阱

- ⚠️ kprobe 的 `argN` 类型恒为 u64——访问结构体成员必须先 `(struct xxx *)arg0` 强转，忘了会编译错或读错数据；
- ⚠️ software/hardware 不带 count 时用默认间隔；高频事件务必显式采样。
- ⚠️ `sys_exit_*` 的 `args->ret` 负值是 -errno，直方图/求和前先过滤。

<details>
<summary>自测题</summary>

1. clone(2) 跟踪点为何"进入 1 次返回 2 次"？
   <details><summary>答案</summary>父子进程各自从 clone 返回一次：父进程返回子 PID，子进程返回 0。</details>

2. profile:hz:99 为什么用 99 而不是 100？
   <details><summary>答案</summary>避免与周期性事件对齐导致的锁定步进采样偏差。</details>

3. kprobe 的 arg0 类型是什么？怎么访问结构体成员？
   <details><summary>答案</summary>恒为 64 位无符号整型；先 `(struct xxx *)arg0` 强转再 `->` 访问。</details>
</details>
