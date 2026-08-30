# 6.3 BPF 工具（五）：系统调用 — syscount / argdist / trace

> 底本：《BPF之巅》第 6 章 CPU，6.3.10–6.3.11 节（印刷 p236–242）。三段式递进：syscount 发现高频调用 → argdist 统计参数分布 → trace 逐事件打印。

## 6.3.10 syscount — 系统调用计数

按类型（和进程）统计系统调用次数 — 用于调查系统 CPU 时间偏高的问题。

```bash
# syscount -i 1
[00:04:18]
SYSCALL      COUNT
futex        152923     ← 线程池锁等待
read         29973
epoll_wait   27865
write        21707
epoll_ctl     4696
```

`-P` 按进程分解：书例 java 进程每秒 ~30 万次系统调用，但只占 48-CPU 系统的 1.6% sys 时间。

- 使用 **`raw_syscalls:sys_enter`** 跟踪点（单个探针看到全部调用）而非 316 个 `syscalls:sys_enter_*`（逐个注册慢）；代价是只有调用 ID，BCC 用 `syscall_name()` 库函数换名字
- **开销实测**：作者构造单 CPU 320 万次调用/s 的压测，性能下降 30% → 换算到生产（48 CPU、30 万次/s，每 CPU 才 6000 次/s）≈ **0.06%**，可忽略
- **strace(1) 警告**：基于 ptrace 的 strace 可让应用性能**跌到不足原来的 1%** — 只有 BPF 工具满足不了才考虑

BCC 选项：`-i interval`、`-d duration`、`-T TOP`（前 N）、`-L`（**统计调用总耗时/延迟**）、`-P`（按进程）、`-p PID`。

bpftrace 单行（用通配探针，慢但直观）：

```bash
bpftrace -e 't:syscalls:sys_enter_* { @[probe] = count(); }'
```

## 6.3.11 argdist 和 trace — 系统调用下钻

syscount 发现高频调用后的两件下钻工具（第 4 章已介绍，这里是应用实例）。

**第一步：查跟踪点参数名**（argdist 用跟踪点必须知道参数名）：

```bash
# tplist -v syscalls:sys_enter_read
syscalls:sys_enter_read
    int syscall_nr;
    unsigned int fd;
    char *buf;
    size_t count;
```

**第二步：argdist 统计参数分布**（内核态聚合，适合高频调用）：

```bash
# argdist -H 't:syscalls:sys_enter_read():int:args->count'   # 请求读取量
    16 -> 31    : 384    ← 两峰：小包协议 + 1KB 级读取
    1024 -> 2047: 267

# argdist -H 't:syscalls:sys_exit_read():int:args->ret'      # 实际返回量
    0 -> 1      : 481    ← 大量 0/1 字节返回（EAGAIN 或空读）
```

请求量 vs 返回量对比 → 发现"要 1KB 只回 1 字节"的浪费模式。`-C` 选项输出频率计数。

**第三步：trace 逐事件打印**（低频调用；显示每个事件的时间戳与细节）。

bpftrace 等价物：

```bash
bpftrace -e 't:syscalls:sys_enter_read { @ = hist(args->count); }'
bpftrace -e 't:syscalls:sys_exit_read { @ = hist(args->ret); }'
```

bpftrace 特性：负值有独立区间 `(. , 0)` — read 返回负数 = 错误。可进一步统计错误码分布：

```bash
bpftrace -e 't:syscalls:sys_exit_read /args->ret < 0/ { @ = lhist(-args->ret, 0, 100, 1); }'
# 输出全部落在 [11,12] → errno 11 = EAGAIN（Try again）
# —— 非阻塞 fd 的正常行为，不是 bug
```

## 工具选择决策

| 场景 | 工具 |
|------|------|
| 哪个系统调用高频 | syscount |
| 高频调用的参数/返回值分布 | argdist（内核态直方图） |
| 低频调用逐事件细节 | trace（打印每个事件） |
| 错误码分布 | argdist / bpftrace lhist(负返回值) |
| 参数名忘了 | tplist -v |

## HFT 关联

- 收单路径 syscount 检查：理想状态 futex/epoll 极少；futex 15 万次/s = 线程间锁等待（书例的 java 就是典型），是无锁化改造的直接证据
- `argdist` 对 `recvfrom` 返回值做直方图 → 看行情包大小分布，验证组播扇出是否正常
- EAGAIN 直方图技巧在生产极有用：非阻塞 socket 上 EAGAIN 是正常噪声，但要区分 EAGAIN（正常）与 EINTR/ECONNRESET（异常）
- **永远不要对交易进程跑 strace** — 性能掉 99%，等于停服；BPF 方案开销 0.1% 以下

## 常见陷阱

1. **对生产进程 strace** — ptrace 断点机制使性能跌至 1% 以下，负载均衡还会触发误迁移
2. **argdist 统计的是"请求量"就下结论** — sys_enter 读的是应用想读多少，真实 I/O 要看 sys_exit 的 ret
3. **read 返回负数被 hist 当普通值** — bpftrace 有独立负值区间，注意 errno 对照（11=EAGAIN、4=EINTR…）
4. **syscount 用 syscalls:sys_enter_* 通配时启动慢** — 316 个探针逐个注册；BCC 版单探针 raw_syscalls 更快（bpftrace 可参照第 14 章 kaddr+syscall_table 技巧）

<details>
<summary>📝 自测题（点击展开）</summary>

1. **syscount 为什么用 raw_syscalls:sys_enter 而不是 syscalls:sys_enter_*？代价是什么？**

   <details>
   <summary>参考答案</summary>

   raw_syscalls 是单个跟踪点，一次注册覆盖所有系统调用，开销和启动速度都优；syscalls:sys_enter_* 要逐个注册 316 个探针。代价：raw 跟踪点只给系统调用 ID（数字），需转换为名字 — BCC 提供 syscall_name() 库函数，bpftrace 要用 kaddr("sys_call_table") + 偏移自己取函数名。
   </details>

2. **argdist 和 trace 分别适合什么场景？**

   <details>
   <summary>参考答案</summary>

   argdist 在内核态做直方图/频率聚合，只输出统计结果，适合每秒数万次以上的高频事件；trace 逐事件打印（时间戳+参数），信息全但输出量与事件数成正比，只适合低频事件的细节调查。
   </details>

</details>
