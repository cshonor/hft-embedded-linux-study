# 结构化故障排查决策树 — 16 SysPerf → 17 BPF 下钻

> **使用方法：** 从症状入口进入，按箭头逐步下钻。
> 标注：`(19)` = SysPerf 传统工具 · `(20)` = BPF 工具 · `(HFT)` = HFT 专项检查

---

## 入口：你看到什么现象？

```
                    ┌──────────────────────────┐
                    │     症状入口              │
                    └──────────┬───────────────┘
           ┌──────────┬───────┴────────┬───────────┐
           ▼          ▼                ▼           ▼
      ① CPU 高    ② 延迟大但       ③ IO 慢     ④ 网络异常
                  CPU 不高
```

---

## ① CPU 利用率高

```
vmstat 1 → %us + %sy 总和高？
│
├─ %us 高（用户态）
│   ├─ (19) perf top → 哪个函数热点？
│   │   ├─ 业务函数 → 算法问题，profiling 优化
│   │   └─ libc / runtime → (20) profile-bpfcc -F 99 30s + 火焰图
│   │       ├─ malloc/free 占比高 → 见 ⑤ 内存
│   │       └─ 锁等待 → 见 ⑥ 锁
│   │
│   └─ (19) pidstat 1 → 哪个进程？
│       └─ 非交易进程 → cgroup 限制 / kill
│
├─ %sy 高（内核态）
│   ├─ (19) perf top → 内核函数热点？
│   │   ├─ softirq / net_rx → 见 ⑦ 网络 softirq
│   │   ├─ futex / spinlock → 见 ⑥ 锁
│   │   ├─ page fault → 见 ⑤ 内存
│   │   └─ ext4/xfs → IO 路径开销，见 ③
│   │
│   └─ (20) profile-bpfcc -F 99 → 内核栈火焰图
│       └─ 定位具体内核函数 → 短跑 bpftrace kprobe 验证
│
└─ %si 高（软中断）
    └─ 见 ⑦
```

**HFT 专项：**
- `(HFT)` 绑核核的 %sy 应 < 5% — 如果高 → 中断落在了交易核
- `(HFT)` 检查 `/proc/interrupts` → 网卡中断是否绑到了交易核

---

## ② 延迟大但 CPU 不高

```
延迟 histogram P99 上涨，但 CPU 利用率 < 50%
│
├─ (19) vmstat 1 → r 列 > 核数？
│   ├─ 是 → 调度饱和 → 见 ②a
│   └─ 否 → 线程在睡眠/等待 → 见 ②b
│
│  ②a 调度饱和
│  ├─ (19) mpstat → 哪个核 runqueue 长？
│  │   └─ (20) runqlat-bpfcc 10 → 等待延迟直方图
│  │       ├─ P99 < 50us → 不太可能是调度问题，继续 ②b
│  │       ├─ P99 > 100us → 邻居进程干扰
│  │       │   └─ (20) runqslower-bpfcc 1 → 抓 > 1ms 的线程
│  │       └─ 某核独高 → cgroup 配额 / CPU affinity 检查
│  │
│  └─ (HFT) 检查 cpuset / taskset 是否生效
│
│  ②b 线程在睡眠
│  ├─ (20) offcputime-bpfcc 30 → off-CPU 栈
│  │   ├─ futex_wait → 锁竞争，见 ⑥
│  │   ├─ epoll_wait → 等网络/IO，见 ③ 或 ④
│  │   ├─ io_schedule → IO 等待，见 ③
│  │   └─ nanosleep → 业务逻辑 sleep（代码问题）
│  │
│  └─ (20) bpftrace: tracepoint:syscalls:sys_enter_* 逐 syscall 延迟
│      └─ 哪个 syscall 最慢 → 针对性下钻
```

**HFT 专项：**
- `(HFT)` 延迟突增但 CPU 不变 → 优先查 off-CPU（线程被挂起了）
- `(HFT)` 对齐业务 histogram 时间窗 → offcputime 采集窗口需覆盖突增时刻

---

## ③ IO 慢 / 磁盘延迟高

```
iostat -x 1 → await 上涨 或 %util 高
│
├─ (19) iostat → 哪个设备？
│   ├─ %util 高 + await 高 → 设备饱和
│   │   ├─ (20) biolatency-bpfcc -D 10 → 延迟直方图
│   │   │   ├─ 双峰 → 部分 hit cache，部分走介质
│   │   │   └─ 右尾长 → 长尾 outlier
│   │   │
│   │   ├─ (20) biotop-bpfcc → 谁在打满磁盘？
│   │   │   ├─ 日志进程 → 异步日志 / 降频
│   │   │   ├─ swap → 内存不够，见 ⑤
│   │   │   └─ 未知 → (20) biosnoop-bpfcc 逐条查
│   │   │
│   │   └─ (19) 查 I/O scheduler / queue depth
│   │
│   ├─ %util 低 + await 高 → 队列堆积（非设备瓶颈）
│   │   ├─ 检查 nr_requests / scheduler（mq-deadline vs none）
│   │   └─ (20) bpftrace block tracepoint → 排队 vs 设备延迟拆分
│   │
│   └─ (HFT) 热路径不应有 IO
│       ├─ 检查 mmap 页是否被换出 → /proc/<pid>/smaps
│       ├─ 检查是否有异步日志写盘 → 改 buffer / offload
│       └─ 检查 mlockall 是否生效
```

---

## ④ 网络异常 / 重传 / 延迟

```
netstat -s → retransmits 增长？ 或 ping 延迟抖动？
│
├─ (19) netstat -s → 哪类问题？
│   ├─ retransmits 增长
│   │   ├─ (20) tcpretrans-bpfcc 30 → 逐条重传事件
│   │   │   ├─ 集中某目的 IP → 对端慢或网络抖动
│   │   │   ├─ 集中 SYN → backlog 溢出 / SYN flood
│   │   │   └─ 随机散布 → 网络质量 / buffer 不足
│   │   │
│   │   └─ (19) ss -tin → 查看 RTT / cwnd / retrans
│   │       └─ rtt 高 → 物理网络问题
│   │
│   ├─ packet drop
│   │   ├─ (19) netstat -s → 查 drops 计数
│   │   ├─ (19) ss -s → socket buffer 满？
│   │   └─ (20) bpftrace tracepoint:skb:kfree_skb → 丢包点
│   │       └─ 定位丢包发生在哪一层（qdisc / driver / protocol）
│   │
│   └─ 延迟抖动（无重传但 RTT 波动）
│       ├─ (19) ping / tcpping → 基础 RTT
│       ├─ (20) bpftrace tcp probe → TCP 栈各阶段耗时
│       └─ (HFT) 检查网卡 coalescing / interrupt moderation
│           └─ 关闭 adaptive, 设为最低延迟
│
└─ (HFT) 行情连接
    ├─ 检查 SO_RCVBUF / SO_SNDBUF 是否足够
    ├─ 检查网卡拉包模式（NAPI / busy polling）
    └─ 检查是否有 conntrack 表满 → /proc/sys/net/netfilter/
```

---

## ⑤ 内存问题 / OOM / 泄漏

```
free -m → available 下降？ 或 /proc/meminfo → Slab 增长？
│
├─ RSS 持续增长（泄漏）
│   ├─ (19) pidstat -r 1 → 哪个进程？
│   ├─ (20) memleak-bpfcc <pid> → 分配未释放的栈
│   └─ (19) /proc/<pid>/smaps → 详细映射
│
├─ Slab 膨胀
│   ├─ (19) slabtop → 哪个 cache？
│   ├─ (20) bpftrace kprobe:__kmalloc → 分配热点栈
│   └─ 检查 dentry / inode cache 是否过多（文件句柄泄漏）
│
├─ page cache 挤压
│   ├─ (19) cachestat 或 sar -B → major fault 计数
│   ├─ (20) bpftrace page_fault tracepoint → 见脚本 ⑥
│   └─ (HFT) 热路径 major fault = 0
│       └─ 检查 mlockall / pre-touch / hugepage
│
├─ cgroup 内存节流
│   ├─ (19) cat /proc/pressure/memory → stall 统计
│   ├─ (19) systemd-cgtop 或 /sys/fs/cgroup/.../memory.events
│   └─ 调大 cgroup memory limit 或优化内存使用
│
└─ NUMA 跨节点
    ├─ (19) numastat -p <pid> → 本地 vs 远端访问
    └─ (HFT) 热路径内存应在本地 NUMA → numactl --membind
```

---

## ⑥ 锁竞争

```
perf top → futex / spinlock 占比高？ 或 offcputime 显示 futex_wait？
│
├─ (20) bpftrace futex 脚本（见脚本集 ⑤）→ 等待时长分布
│   ├─ < 10us → 自旋快速获取，可接受
│   ├─ 100us–1ms → 真实竞争
│   │   ├─ (20) bpftrace kprobe:futex_wait → 持锁者是谁
│   │   └─ 代码审查：拆锁 / 减小临界区
│   └─ > 1ms → 持锁者做了慢操作
│       └─ (20) offcputime-bpfcc → 持锁线程的 off-CPU 栈
│
├─ 内核锁（spinlock）
│   ├─ (20) bpftrace kprobe:__raw_spin_lock → 争抢热点
│   └─ 通常意味着内核路径慢（IO / memory reclaim）
│
└─ (HFT) 热路径不应有锁
    ├─ 无锁队列检查 → 是否误用了 mutex
    ├─ 检查 seqlock / RCU 是否更适合读多场景
    └─ 检查 thread-affinity 是否导致锁跨核传递
```

---

## ⑦ 软中断 / 网络包处理

```
mpstat -I SUM 1 → %soft 高？
│
├─ (19) /proc/interrupts → 中断落在哪个核？
│   ├─ 落在交易核 → 绑核错误 → 修正 smp_affinity
│   └─ 落在网络核 → 正常，继续查 softirq 耗时
│
├─ (20) bpftrace softirq 脚本（见脚本集 ⑧）→ 处理时长
│   ├─ P99 < 100us → 健康
│   └─ 右尾到 ms → 单次处理太多包
│       ├─ 检查 RPS / RFS 是否将负载分散
│       ├─ 检查 NAPI weight
│       └─ (HFT) 考虑 busy polling (SO_BUSY_POLL)
│
└─ (HFT) 行情核 vs 网络核分离
    ├─ cpuset 隔离交易线程
    ├─ 网卡中断绑到非交易核
    └─ 确认 IRQ affinity 不会被 irqbalance 打乱
```

---

## 快速对照表：症状 → 第一工具

| 症状 | 第一步 (19) | 第二步 (20) | HFT 专项 |
|------|------------|------------|----------|
| CPU 高 | `perf top` | `profile-bpfcc -F 99` | 绑核核 %sy < 5% |
| 延迟大 CPU 不高 | `vmstat 1` | `offcputime-bpfcc` | 对齐业务 histogram |
| IO 慢 | `iostat -x 1` | `biolatency-bpfcc -D` | 热路径 0 IO |
| 网络重传 | `netstat -s` | `tcpretrans-bpfcc` | 行情连接检查 |
| 内存泄漏 | `pidstat -r 1` | `memleak-bpfcc <pid>` | major fault = 0 |
| 锁竞争 | `perf top` | `bpftrace futex 脚本` | 热路径无锁 |
| 软中断高 | `mpstat -I` | `bpftrace softirq 脚本` | 中断核 ≠ 交易核 |

---

## 下钻原则

1. **先传统后 BPF** — 19 的工具看现象（便宜、低风险），20 的工具钻根因（贵、有开销）
2. **先统计后栈** — 先 `hist()` / `count()` 看分布，确认有问题再 `kstack` 钻栈
3. **先全局后局部** — 先不限 PID 看全貌，再 `pid == XXX` 聚焦
4. **限时限量** — 每次采集 ≤ 60s，生产环境必须加 `timeout`
5. **热路径核慎挂** — 交易核心 CPU 不挂探针，只挂非交易核
