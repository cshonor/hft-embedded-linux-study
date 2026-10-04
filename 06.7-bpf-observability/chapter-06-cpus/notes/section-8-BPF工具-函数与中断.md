# 6.3 BPF 工具（六）：函数调用与中断 — funccount / softirqs / hardirqs / smpcalls

> 底本：《BPF之巅》第 6 章 CPU，6.3.12–6.3.15 节（印刷 p242–250）。

## 6.3.12 funccount — 函数调用频率

第 4 章工具在 CPU 分析的应用。**profile 告诉你哪个函数占 CPU，但不解释为什么** — 是函数本身慢，还是每秒被调用几百万次？funccount 补上后半段。

```bash
# funccount 'tcp_*'                  # 统计 36 个 tcp 开头内核函数
tcp_md5_do_lookup    510322          ← 调用最频繁（MD5 认证查表）

# funccount -i 1 get_page_from_freelist
get_page_from_freelist  586452       ← 每秒 58 万次
get_page_from_freelist  586241
```

书例闭环：profile 火焰图显示 get_page_from_freelist 占 CPU 最宽 → funccount 发现它每秒被调 58 万次 → 结论是**调用频繁**（页分配压力）而非单次执行慢。

- 原理：kprobes（内核函数）/ uprobes（用户态函数）动态跟踪，**开销与调用频率成正比**
- ⚠️ malloc、get_page_from_freelist 这类超高频函数，跟踪可致性能损失 **>10%** — 小心使用（开销计算见 18.1 节）

模式语法：`name` / `p:name`（内核函数）、`lib:name` / `path:name`（用户态）、`t:system:name`（跟踪点）、`*` 通配；选项 `-r`（正则）、`-i interval`、`-d duration`、`-p PID`。

bpftrace：

```bash
bpftrace -e 'kprobe:tcp_* { @[probe] = count(); }'
interval:s:1 { print(@); clear(@); }     # 定时输出
```

## 6.3.13 softirqs — 软中断耗时

BCC 工具，显示各软中断的**处理总耗时**（不只是计数 — 这是与 mpstat %soft、/proc/softirqs 的区别）。

```bash
# softirqs 10 1
SOFTIRQ   TOTAL_uSecs
net_rx    1358268      ← 1358ms/10s，48 核系统 ≈ 3% CPU
timer      389144
sched      185873
rcu        143859
tasklet     30939
net_tx         633
```

- 内部用 `irq:softirq_entry` / `irq:softirq_exit` 跟踪点；网络繁忙环境开销显著，小心
- `-d` 直方图模式（看耗时分布，抓超长处理）、`-T` 时间戳
- 无 bpftrace 版（原书练习 10）；单行起点：`t:irq:softirq_entry { @[args->vec] = count(); }`，向量 ID 查表换名字，计时需配 softirq_exit

## 6.3.14 hardirqs — 硬中断耗时

BCC 工具，各硬中断**处理总耗时**（mpstat %irq 与 /proc/interrupts 只有计数）。

```bash
# hardirqs 10 1
HARDIRQ              TOTAL_usecs
eth0-Tx-Rx-1             52649    ← 网卡队列中断各 ~50ms
eth0-Tx-Rx-4             51106
...
nvme0q0                     46
ena-mgmnt@pci:...          43
```

- **性能剖析器看不到的时间**：硬中断是"偷走"的 CPU 时间，profile 采样中体现为被打断的任意栈 — hardirqs 直接量化（无 PMU 云环境的重要补充，见 6.2.4）
- 动态跟踪 `handle_irq_event_percpu()`（kprobe）；未来会切到 `irq:irq_handler_entry/exit` 跟踪点
- `-d` 直方图、`-T` 时间戳

## 6.3.15 smpcalls — SMP 跨 CPU 调用（bpftrace）

本书新作（2019-01）：统计 SMP 调用（跨 CPU 调用）耗时 — 一个 CPU 让其他 CPU 代为执行函数，多核系统上可能很贵。

```bash
# smpcalls.bt
@time_ns[do_flush_tlb_all]:        [64K, 128K]     ← TLB 全刷 128µs
@time_ns[remote_function]:         [32K, 64K]
@time_ns[native_smp_send_reschedule]: [16K, 32K]   ← 47 个样本
@time_ns[aperfmperf_snapshot_khz]: [64K, 128K]     ← 作者第一次运行就抓到的元凶
```

**教科书级排查案例**（值得背下来）：

1. smpcalls 发现 `aperfmperf_snapshot_khz` 耗时 128µs 且高频
2. 改直方图键加上 `comm, kstack` → 锁定 snmp-pass 进程，栈：open()→cpuinfo_open→aperfmperf_snapshot_khz
3. 用 opensnoop 交叉验证：snmp-pass **每秒读一次 /proc/cpuinfo**
4. 查源码：它只是想数 CPU 个数，"cpu MHz" 根本没用 — 典型无用功，删掉即得性能提升

实现要点：

- 大部分调用走 `smp_call_function_single()` / `smp_call_function_many()` 的 kprobe；**arg1 是要在远端 CPU 执行的函数指针**，kretprobe 中用 `ksym()` 换名字
- 特例 `smp_send_reschedule()` 不经过上面两个函数 → 单独 kprobe `native_smp_send_reschedule`，用 `reg("ip")` 取 IP
- Intel 上 SMP 调用最终落地为 x2APIC IPI（跨处理器中断），可跟踪 `x2apic_send_IPI*`（见 6.4 单行）
- 期望未来内核加 SMP 调用跟踪点，简化跟踪

## HFT 关联

- **网卡多队列中断亲和**：hardirqs 输出中 eth0-Tx-Rx-* 的分布应与 IRQ affinity 设置一致 — 全堆在策略核上就是事故
- SMP 调用 = 核间打断。`do_flush_tlb_all` 高频出现（如频繁 munmap 触发 TLB shootdown）会同时打断所有隔离核 — smpcalls 直接量化；解法是减少无谓的 mm 操作
- snmp-pass 案例的通用启示：**监控工具自己可能就是干扰源**（每秒读 /proc/cpuinfo 引发跨核 IPI）— 交易机上审计所有监控 agent 读 /proc 的频率
- funccount 是验证"函数改对了没"的利器：热路径函数优化前后各跑一次 -i 1 对比调用率

## 常见陷阱

1. **funccount 跟踪 malloc/get_page 类超高频函数** — 开销 >10%，生产慎用；先用 profile 确认目标再定点 funccount
2. **把 softirqs/hardirqs 计数当耗时** — mpstat%/proc 的计数只是次数；一次 100µs 的中断和 10 次 1µs 的中断计数相近、影响天差地别
3. **profile 看不到中断时间就当中断没问题** — 硬中断时间是"隐身"的，必须 hardirqs 量化
4. **smpcalls 只看函数名不看调用栈** — 不加 kstack 键就找不到是谁发起的（snmp-pass 案例的关键一步）

<details>
<summary>📝 自测题（点击展开）</summary>

1. **profile 显示函数 A 占 CPU 最多，如何判断它慢还是被调太勤？**

   <details>
   <summary>参考答案</summary>

   funccount -i 1 统计 A 的每秒调用次数。若每秒几十万次 → 调用频繁是主因（优化调用方/减少调用）；若低频但占 CPU → 单次执行慢（剖析函数内部）。书例 get_page_from_freelist 每秒 58 万次 = 页分配风暴。
   </details>

2. **snmp-pass 案例的完整排查链条是什么？**

   <details>
   <summary>参考答案</summary>

   smpcalls.bt 发现 aperfmperf_snapshot_khz 耗 128µs → 给直方图键加 comm+kstack → 锁定 snmp-pass 的 open() 调用栈 → opensnoop 验证它每秒读 /proc/cpuinfo → 读源码发现只是数 CPU 个数 → 删除该读取即修复。方法论：BPF 定位热点 → 加栈定位发起者 → 传统工具交叉验证 → 审代码找根因。
   </details>

3. **为什么 smp_send_reschedule 要单独跟踪而不能用 smp_call_function_*？**

   <details>
   <summary>参考答案</summary>

   它不经过 smp_call_function_single/many 路径（是特殊的快速 IPI 通知），所以要用 kprobe:native_smp_send_reschedule 单独挂，函数名用 reg("ip") 从指令指针获取。
   </details>

</details>
