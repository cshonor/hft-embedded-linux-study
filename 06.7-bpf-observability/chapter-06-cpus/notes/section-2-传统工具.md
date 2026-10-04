# 6.2 传统工具

> 底本：《BPF之巅》第 6 章 CPU，6.2 节（印刷 p198–210）。五个层次：内核统计 → 硬件统计 → 硬件采样 → 定时采样 → 事件统计与跟踪。BPF 工具登场前必须先掌握这套"发射前检查"基线。

## 6.2.1 内核统计

| 工具 | 要点 |
|------|------|
| `uptime` | 负载平均值 = 1/5/15 分钟**指数衰减累加**，包含**不可中断**（D 状态）任务 → 高负载 ≠ CPU 瓶颈 |
| `top` / `htop` | `%CPU` 单进程可超 100%（多核之和）；采样周期内短命进程完全不可见 |
| `mpstat -P ALL 1` | 按 CPU 分别输出 — 识别**负载不均**（绑核/中断亲和错误时某几个核 100%） |
| `vmstat 1` | `r` 列 = 运行队列长度（含正在运行）；`cs` = 上下文切换次数/秒 |
| `sar -uq 1` | 同时看利用率（-u）和队列（-q runq-sz/ldavg）— 6.3.3 中验证编译机超载的交叉工具 |

## 6.2.2 硬件统计（PMC 计数模式）

```bash
perf stat -d -a -- sleep 10      # 全系统，含派生指标
perf list | grep -i cache        # 本机可用 PMC 事件
perf stat -e LLC-loads,LLC-load-misses -a -I 1000
```

- PMC（性能监控计数器）由 CPU 硬件维护，读取代价极低
- `perf stat -d` 给出 IPC（每周期指令数）：**IPC < 1.0 提示 stall**（缓存未命中/分支预测失败等），HFT 低延迟路径理想值应远高于 1
- **`tlbstat`**（perf-tools）：统计 TLB 未命中。书中 KPTI（Meltdown 缓解页表隔离）案例：DTLB 未命中 27% + ITLB 22% — 内核页表隔离让每次系统调用/中断都要刷 TLB，代价巨大
- 云主机常无 PMC 直通（`perf stat` 报错或全 0）→ 退路是 BPF 工具（见 6.3.14 hardirqs 提示与 6.2.4）

## 6.2.3 硬件采样（PMC 采样模式）

```bash
perf record -e mem_load_retired.l3_miss -c 50000 -a -- sleep 10
```

- 按计数器**溢出采样**：每 N 次事件触发一次记录，把指令指针、调用栈存下来
- `-c 50000` = 每 5 万次事件采一个样本；**PEBS**（Intel）能记录溢出**精确时刻**的指令指针，消除 interrupt skid
- 用途：定位缓存未命中的代码行 — 比 IPC 更进一步回答"哪里 stall"

## 6.2.4 定时采样（性能剖析）

```bash
perf record -F 99 -a -g -- sleep 30
perf report          # 或 perf script 出原始数据
```

- `-F 99`：**99Hz 而非 100Hz** — 避免与系统中其他 100Hz 周期活动锁定步进（采偏样本）
- `-g`：记录调用栈（需 frame pointer；`-` callchain 用 DWARF 但开销大）
- 输出可折叠（`perf script | stackcollapse-perf.pl | flamegraph.pl`）生成 **CPU 火焰图**
- **无 PMU 的云环境**：perf 退化为 hrtimer 软中断采样 — 仍可用但有偏差；BPF `profile`（6.3.8）同样基于 perf_event 但在内核态完成频率统计，开销更低

## 6.2.5 事件统计与跟踪

```bash
perf stat -e sched:sched_process_exec -I 1000     # 统计新进程 exec 频率
perf sched timehist                               # 调度事件时间线（离线分析）
```

- 大量 `sched_process_exec` = 短命进程风暴（cron/脚本循环）— top 看不见，这里能数出来
- `perf sched timehist` 记录每次调度事件后离线分析 **runqueue 情况，但跟踪开销高**，书中明确建议：这类需求改用 BPF 的 `runqlat`/`runqlen`
- Ftrace `funccount`（perf-tools）也可统计函数调用频率，是 6.3.12 BCC funccount 的前身

## HFT 关联

- 交易机基线巡检：`mpstat -P ALL 1` 看核间均衡 + `vmstat 1` 看 `cs`/`r` — 一分钟内出结论
- KPTI/tlbstat 案例是 HFT 的直接教训：**安全缓解措施可吃掉两位数性能**，部署前用 tlbstat 验证
- `perf record -F 99` 的 99Hz 原则同样适用于 bpftrace `profile:hz:99`

## 常见陷阱

1. **负载平均值含 D 状态任务** — 高 load + 低 CPU 使用率说明瓶颈在 I/O 或锁，不是 CPU
2. **top 的 %CPU 汇总掩盖短命进程** — 每 3 秒采样一次，存活 50ms 的进程永远抓不到（execsnoop 的存在意义）
3. **云主机 perf stat PMC 全 0** — hypervisor 未透传 PMC，别误判为"无缓存问题"；换 BPF 工具
4. **perf sched timehist 常驻** — 调度事件跟踪开销高，生产环境用 runqlat 替代

<details>
<summary>📝 自测题（点击展开）</summary>

1. **负载平均值为 32，但 CPU 使用率只有 20%，说明了什么？**

   <details>
   <summary>参考答案</summary>

   负载含不可中断（D 状态）任务，也含 RUNNABLE 排队任务。CPU 使用率低 + 高负载 → 要么大量线程阻塞在 I/O/锁（D 状态），要么运行队列积压但瓶颈在别处。用 vmstat r 列、`ps -eo state` 统计 D 状态数量进一步定位。
   </details>

2. **perf record 为什么用 99Hz 而不是 100Hz？**

   <details>
   <summary>参考答案</summary>

   避免采样与系统中以 100Hz 为周期的活动（如某些定时器）锁定步进 — 若同相采样，每次都采到同一个周期函数的开头/结尾，产生系统性偏差。99Hz 让采样点"漂移"过所有相位。
   </details>

</details>
