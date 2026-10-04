# 6.3 BPF 工具（三）：CPU 执行时长与频率 — cpudist / cpufreq

> 底本：《BPF之巅》第 6 章 CPU，6.3.6–6.3.7 节（印刷 p223–227）。

## 6.3.6 cpudist — 线程在 CPU 上执行时长分布

**测量**：每次线程唤醒后**在 CPU 上连续执行多久**（on-CPU time），直方图。回答"线程拿到的 CPU 是细碎的还是整块的" — 定性分析 CPU 使用率，为优化决策提供依据。

书例一（48-CPU 生产机，默认 µs 档）：高峰在 **0–127µs** — 线程频繁被切走。

书例二（同机 `-m` 毫秒档）：出现 **4–15ms** 高峰 — 很可能正好是**调度器时间片长度**：线程用满配额后被**被动上下文切换**。

实战案例（Netflix）：一次变更让 ML 程序快了 3 倍 — `perf` 只看到上下文切换次数下降，**cpudist 解释了机制**：变更前每线程只能跑 0–3µs 就被切走（碎片化），变更后拿到整块时间。

原理与开销：

- 跟踪 `sched:sched_switch`，记录上线→下线的时间差
- 繁忙系统切换每秒可超百万次 → **开销可能显著（>10% 量级），短期运行**

BCC 选项：`-m`（毫秒）、`-O`（**off-CPU 时间**，即离核时长而非在核时长）、`-P`（每进程直方图）、`-p PID`。

> 无 bpftrace 版 — 原书留作练习（section-11 练习 11）。

## 6.3.7 cpufreq — CPU 频率采样

**采样**（100Hz）每个 CPU 当前频率，可按进程名分别输出直方图。只在**支持调频的 governor**（如 powersave）下有意义；performance 模式下频率恒定、无频率事件，**工具无输出**（这本身就是个诊断信号）。

书例解读：

```
@process_mhz[snmpd]:   [1200, 1400]     ← 空闲，低频
@process_mhz[python3]: [3000, 3200]     ← 全功率
@process_mhz[java]:    [1200, 1400] 主峰，仅 18 个样本到 [3000,3200]
                          ← java 大部分时间等磁盘 I/O，CPU 省电模式
@system_mhz:           [1200, 1400] 22041 样本 ← 系统整体空闲
```

生产实锤：nginx 在 powersave governor 下大部分时间低频运行 — **默认 governor 是 powersave 而非 performance**，性能白白损失。governor 由 `/sys/devices/system/cpu/cpufreq/*/scaling_governor` 设置。

bpftrace 源码（跟踪点 + 采样结合的范例）：

```bash
#!/usr/local/bin/bpftrace
BEGIN { printf("Sampling cpu freq system-wide & by process. Ctrl-c to end.\n"); }

tracepoint:power:cpu_frequency
{
    @curfreq[cpu] = args->state;       // 频率变化时更新当前值
}

profile:hz:100
/@curfreq[cpu]/
{
    @system_mhz = lhist(@curfreq[cpu]/1000, 0, 5000, 200);
    if (pid)
        @process_mhz[comm] = lhist(@curfreq[cpu]/1000, 0, 5000, 200);
}

END { clear(@curfreq); }
```

- `power:cpu_frequency` 跟踪点记录频率变化，profile 探针 100Hz 采样 → 开销可忽略
- 直方图范围 0–5000MHz、每 200MHz 一档，可改源码调整
- `if (pid)` 过滤 idle 线程（pid==0）

## HFT 关联

- **交易机必须 performance governor**：cpufreq 无输出 = 频率锁定最高 = 正确状态；有输出且主峰低频 = 灾难（行情处理突然掉频）
- 云实例更要验证：hypervisor 的 turbo/降频策略不受 guest 控制，同型号机器实测频率可能不同 → cpufreq.bt 进基线检查
- cpudist 用于验证绑核效果：策略线程的 on-CPU 时长应是"整块毫秒级"，若碎片化说明被同核干扰

## 常见陷阱

1. **performance 模式下 cpufreq 无输出误判为故障** — 无频率变化事件是正常现象，恰好证明频率恒定
2. **powersave governor 名字有误导** — 现代 Intel CPU 上 powersave 也允许按负载升频，但不保证及时；延迟敏感负载别赌它
3. **cpudist 默认微秒档看毫秒现象** — 时间片级（毫秒）模式必须加 `-m`，否则挤在最左桶
4. **忙系统长跑 cpudist** — sched_switch 事件跟踪每秒百万次级，开销可观，限定窗口 + `-p PID`

<details>
<summary>📝 自测题（点击展开）</summary>

1. **cpudist 出现 4–15ms 的峰，最可能是什么造成的？**

   <details>
   <summary>参考答案</summary>

   调度器时间片耗尽导致的被动上下文切换：线程用满 CFS 分配的运行配额（默认在毫秒量级）后被抢占。说明这些线程是 CPU 密集型而非 I/O 阻塞型。
   </details>

2. **为什么 cpufreq 在 performance governor 下没有输出？**

   <details>
   <summary>参考答案</summary>

   它依赖 power:cpu_frequency 跟踪点 — 只有频率变化才触发事件。performance 模式频率恒为最大值，从不变化，无事件可跟踪。所以"工具没输出"本身是频率已锁定的证据。
   </details>

</details>
