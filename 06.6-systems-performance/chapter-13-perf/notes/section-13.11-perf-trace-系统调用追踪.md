# 13.11 `perf trace` — 系统调用追踪

> [章节导航](../README.md) · 上一节：[13.10 perf report 与 script](./section-13.10-perf-report-与-perf-script.md) · 下一节：[13.12 其他能力延伸](./section-13.12-其他常用能力延伸.md)

## 本节讲什么

perf trace 是 syscall 视角的观测器（strace 的 perf 版）。核心内容：

1. **strace 为什么慢**——ptrace 的双陷阱机制
2. **perf trace 为什么快**——tracepoint 单点旁路
3. syscall 汇总模式（`-s`）——开销再降一个量级的统计姿势

---

## 1. 机制对比：ptrace vs tracepoint

### 1.1 strace 的双陷阱

```
strace（ptrace ATTACH）
  目标进程每次 syscall：
    ① 入口：内核 ptrace 停止 → SIGSTOP 唤醒 strace → strace ptrace(PEEKUSRD) 拷参数
       → ptrace(CONT) 恢复目标
    ② 出口：再停止一次 → 再唤醒 strace → 再拷返回值 → 再恢复
```

**每次 syscall 两次进程切换 + 两次上下文往返**——高频 syscall 进程（网络行情接收）直接慢 10~100 倍。ptrace 本质是**调试器协议**，不是观测通道。

### 1.2 perf trace 的单点路径

```
perf trace
  内核 tracepoint：syscalls:sys_enter_read / sys_exit_read（静态埋点，[Ch 14.5](../../chapter-14-ftrace/notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)）
  每次 syscall：埋点把参数/返回值写进 perf ring buffer（mmap 零拷贝，[13.9](./section-13.9-perf-record-剖析采样.md)）
  perf 用户态异步收割打印
```

| | strace | perf trace |
|---|---|---|
| 拦截机制 | ptrace（调试协议） | tracepoint（观测埋点） |
| 每 syscall 成本 | 2×进程切换 + 2×ptrace 往返 | 1 次 buffer 写 |
| 目标进程感知 | 被**串行化**（等 tracer 放行） | 无感，继续跑 |
| 时序 | 强保序 | 大致有序（per-CPU buffer） |
| 输出 | 逐条强制 | 逐条或**汇总** |

**仍要限时长**：tracepoint 每命中一次都写 buffer + perf 用户态解析——几万 syscall/s 的进程上，观测负载仍可观（对照 [13.1 生产纪律表](./section-13.1-13.2-子命令概述与单行命令.md)）。

## 2. 用法

```bash
# 逐条视图（strace 式）
perf trace -p $(pidof strategy) -- sleep 5
perf trace -e open,read,write,mmap -- sleep 3          # 只跟这几类

# ⭐ 汇总模式：syscall 直方统计（不开逐条打印）
perf trace -s -p $(pidof strategy) -- sleep 10
perf trace -s --summary-mode total-call / total-time …

# 延迟视角（每 syscall 耗时分布）
perf trace -s --call-trace …… # 视版本；syscall 耗时 = exit 时间戳 − enter 时间戳
```

### 2.1 `-s` 汇总输出精读

```
 Summary of events (strategy, 10 threads):

   syscall            calls    errors  total min avg max  (msec)
   --------------- --------  --------  ----- --- --- ---
   futex              28103           1  0.002 0.000 0.000 12.3
   epoll_wait          6001           0 45.001 1.102 7.500 61.2
   recvfrom            5999           0  3.210 0.001 0.001 55.3
   read                1200         120  0.980 0.000 0.001 8.9
```

| 列 | HFT 判读 |
|---|---|
| calls | **syscall 密度**——行情进程 recvfrom 6k/10s 正常；read 1200 里 120 errors = 重试风暴（网络不稳/非阻塞 EAGAIN 未处理） |
| avg / max | 平均健康但 max 尖刺 → 尾延迟在内核路径或调度（转 [runqlat](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md)） |
| futex 巨多 | **锁行为画像**——无竞争 futex（无 FUTEX_WAIT 命中）便宜，但暗示共享结构；FUTEX_WAIT 慢 = 争用（转 [perf lock，13.12](./section-13.12-其他常用能力延伸.md)） |
| epoll_wait avg≈7.5ms | **事件驱动节奏**——唤醒粒度即处理粒度；对照 busy-poll 决策（→ [10 网络](../../chapter-10-network/)） |

### 2.2 典型排查：热路径 unexpected syscall

```bash
# 症状：tick 处理偶发 200µs 尖刺，CPU 图无热点
perf trace -s -p $(pidof strategy) -- sleep 10
# 发现：mmap/munmap 600 次 —— 每 tick 动态映射！
# 定位：munmap 触发 TLB shootdown（IPI 广播，[06-linux-mm ch03](../../../06-linux-mm/)）
# 修法：预分配 arena / mmap cache
```

这个模式是 perf trace 的 HFT 最强用法：**syscall 直方图 = 内核交互行为画像**——内存映射、页错误、调度、锁全在里面留痕。

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 上线前 syscall 审计 | 开发机 `perf trace -s` 存一份"健康档案"；生产异常时对比 syscall 集合差异——新出现的 mmap/ioctl 就是嫌疑犯 |
| 延迟分账 | syscall avg/max + [perf stat](./section-13.8-perf-stat-事件计数.md) 内核态周期：内核路径慢（syscall max 大）vs 调度慢（syscall 正常但 off-CPU 大） |
| 锁画像 | futex 调用形态（WAIT vs WAKE 比例、错误率）比源码读锁更快发现问题 |
| 嵌入式 | strace 在弱机上不可用（开销崩）；perf trace + 汇总模式是弱机 syscall 观测的唯一现实选项 |
| 永远别在生产逐条跑 | 逐条模式只做开发机 5 秒级定位；生产只要汇总 |

---

## 衔接

- 上一节：[report/script](./section-13.10-perf-report-与-perf-script.md)
- 下一节：[13.12 perf lock/sched/c2c/mem 延伸](./section-13.12-其他常用能力延伸.md)
- tracepoint 机制：[Ch 14.5 事件源](../../chapter-14-ftrace/notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)
- 更强的 syscall 追踪（参数过滤/聚合）：[Ch 15 bpftrace](../../chapter-15-bpf/)

---

## 代码自测

<details><summary>Q1：strace 慢的根本原因？</summary>

ptrace 调试协议：每次 syscall 在入口和出口各停一次目标进程、唤醒 tracer（两次上下文切换 + ptrace 调用往返拷贝）。目标被 tracer **串行化**——它必须等 strace 放行才能继续。
</details>

<details><summary>Q2：perf trace 的数据通路和 strace 有什么本质不同？</summary>

tracepoint 静态埋点（sys_enter/sys_exit）→ 参数写进 mmap ring buffer（零拷贝）→ perf 异步收割。目标进程**不被拦截**，继续执行；观测是旁路不是关卡。
</details>

<details><summary>Q3：汇总模式（-s）为什么比逐条便宜一个量级？</summary>

逐条模式要为每条 syscall 解码参数 + 格式化打印（用户态 CPU 密集）；汇总只聚合计数/耗时直方——buffer 写入相同，用户态处理从"每条全解析"降为"每条一次桶自增"（与 hist trigger 同一思想，[Ch 14.5](../../chapter-14-ftrace/notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)）。
</details>

<details><summary>Q4：syscall 汇总里 futex 调用极多但耗时不长，说明什么？</summary>

锁的"行为指纹"：多为无竞争的原子路径（futex 无 WAIT 时不进内核等待），性能无害；但暴露了共享结构的密度——真争用（FUTEX_WAIT 耗时/错误）才是要处理的。对照 perf lock 进一步分账。
</details>

<details><summary>Q5：热路径尖刺但 CPU 火焰图干净，perf trace 能提供什么线索？</summary>

CPU 图只覆盖 on-CPU 时间。syscall 汇总暴露内核交互面：偶现的 mmap/munmap（TLB shootdown）、read 错误重试、epoll_wait 长尾（调度等待）——这些"不烧 CPU 但制造延迟"的行为只在 syscall 视角留痕。
</details>
