# 13.9 `perf record` — 剖析采样

> [章节导航](../README.md) · 上一节：[13.8 perf stat](./section-13.8-perf-stat-事件计数.md) · 下一节：[13.10 perf report 与 script](./section-13.10-perf-report-与-perf-script.md)

## 本节讲什么

record 是采样流的核心。原书讲选项——这里把**一次采样的内核全生命周期**拆开：

```
PMC 溢出中断（NMI）
  → __perf_event_overflow()（core.c:9500）
  → irq_work 转移到可安全上下文（:2471）
  → 采集样本（IP/时间戳/调用栈，按 sample_type 位掩码）
  → perf_output_sample()（:7262）写 mmap ring buffer
  → 用户态 perf 周期性收割 perf.data
```

以及栈回溯三种方法的取舍、`-F 99` 的拍频理由、内核限流机制。

---

## 1. 采样触发：两种节拍

| 模式 | 选项 | 语义 | 适用 |
|---|---|---|---|
| **频率** | `-F 99` | 每秒约 99 个样本（内核自动调周期） | 默认推荐 |
| **周期** | `-c 1000000 -e cycles` | 每 100 万周期采一次 | 需要与工作量对齐时 |
| 软件节拍 | `-e cpu-clock -F 99` | 定时器驱动（无 PMC 依赖） | 环境 PMC 不可用时 |

**`-F 99` 而非 100**：避开与 OS timer（100/250/1000Hz）及常见周期任务的**拍频共振**——100Hz 采样会总落在同一相位，周期性任务可能被系统性错过或放大。

**频率模式的实现**：`attr.sample_freq` 写入后，内核在每个样本后按 `period = 计数值/目标频率` 动态重设 PMC 周期（uapi `sample_freq` :404 与 `sample_period` :403 共用 union）。

## 2. ⭐ 一次采样的内核旅程（v6.6 锚点）

### 2.1 溢出中断与 NMI

PMC 溢出触发的中断在 x86 上通常是 **NMI**（不可屏蔽）——因为采样对象可能是内核关中断区，普通中断进不去。NMI 上下文有严格限制（不能睡眠、不能拿锁），所以：

| 步骤 | v6.6 位置 | 说明 |
|---|---|---|
| ① 溢出处理入口 `__perf_event_overflow()` | core.c:9500 | 判断限流、构造样本数据 |
| ② `irq_work_queue(&event->pending_irq)` | core.c:2471 | **把真正的工作推迟到下一个可安全中断上下文**（NMI → 普通 irq_work） |
| ③ `__perf_event_output()` | core.c:7803 | 调 ring buffer 写入 |
| ④ `perf_output_sample()` | core.c:7262 | 按 `sample_type` 位掩码逐字段序列化样本 |

**⭐ 这就是"采样样本之间 skid/时间戳抖动"的机制根源**：NMI → irq_work 的转移本身有延迟窗口，样本时间戳来自溢出时刻（`data->time`）但落 buffer 在稍后。

### 2.2 限流（throttle）

内核防采样风暴的三道闸（core.c）：

| 闸 | 值 | 作用 |
|---|---|---|
| `sysctl_perf_event_sample_rate` | 默认 **100000/s**（:426–430 `DEFAULT_MAX_SAMPLE_RATE`） | 全局频率上限 |
| `max_samples_per_tick` | 100000/HZ 每 tick | 防 tick 内爆发 |
| `perf_sample_allowed_ns` | 换算成 ns | 单事件级限流，超限的样本被丢弃并记 `PERF_RECORD_THROTTLE` |

> `-F 99` 远低于限流线，正常 profiling 碰不到；但 `-e page-faults -c 1`（每次缺页都采）在缺页风暴下会被内核 throttle——**收不到样本先想限流**，不是 bug。

### 2.3 ring buffer：mmap 出来的生产者-消费者环

`perf record` 启动时 `mmap(fd)` 得到一块共享内存，布局：`user_page + data 环形区`。无锁协议（ring_buffer.c:58–98 注释原文，经典的一读一写指针设计）：

```
内核（生产者）                    用户态（消费者）
  本地变量 head                     本地变量 tail
  LOAD  rb->user_page->data_tail    LOAD  rb->user_page->data_head
  写样本数据到 head                 读 [tail, head) 区间
  STORE rb->user_page->data_head    消费完 STORE rb->user_page->data_tail
```

| 细节 | v6.6 锚点 | 意义 |
|---|---|---|
| 内核写侧 | `__perf_output_begin()`（:149），读 data_tail :197 | 环满时的丢样本策略 |
| 发布写指针 | `WRITE_ONCE(rb->user_page->data_head, head)` :110 | **唯一共享可变状态**——release 语义 |
| 内存屏障 | :88–98 大段注释 | 保证"消费者先见 head 后见数据"的顺序 |

**⭐ HFT 视角**：这是**用户态与内核最高效的数据通道**（零拷贝、无系统调用、无锁）——与 DPDK 的 ring、[io_uring 的 SQ/CQ](../../../03-linux-userspace-api/) 同一设计家族。perf 99Hz 下每秒 99 条毫无压力；调到万级频率也撑得住，瓶颈在采样本身的 NMI/irq_work 成本而非传输。

## 3. 栈回溯（`-g`）

| 方法 | 原理 | 要求 | 取舍 |
|---|---|---|---|
| **fp**（帧指针） | 沿 RBP 链逐帧走 | 编译 `-fno-omit-frame-pointer` | 快、通用；**牺牲一个寄存器**（~1-2% 性能） |
| **dwarf** | 展开 .eh_frame/.debug_info | 二进制带 debuginfo | 准；每样本拷贝数 KB 栈 + 用户态展开，**慢且 perf.data 巨大** |
| **lbr** | 硬件 Last Branch Record 栈 | Intel CPU | 快且准；栈深受限（32 层）、老 AMD 无 |

```bash
perf record -F 99 -g --call-graph fp -p $(pidof strategy) -- sleep 30
```

**HFT 生产构建的默认立场**：**Release 保留 `-g -fno-omit-frame-pointer`**。1-2% 的寄存器开销换"任何时候都能拿到可读火焰图"——尾延迟事故的归因价值远超这点成本（[Ch 5 Gotchas](../../chapter-05-applications/)）。

**[unknown] 诊断表**：

| 症状 | 原因 | 修法 |
|---|---|---|
| 全是 `[unknown]` | 符号被 strip | 装 debuginfo / 勿 strip |
| 栈只有 2-3 帧就断 | fp 被优化掉 | 编译选项 / 试 dwarf |
| 某几个函数无栈 | `-O3` inline 重灾区 | `--call-graph dwarf` 或 `__attribute__((noinline))` 热点函数 |

## 4. 关键选项速查

| 选项 | 含义 | 备注 |
|---|---|---|
| `-F N` | 目标频率 | 99 常规；499 短窗口细查 |
| `-c N` | 周期触发 | 与 -F 互斥 |
| `-g` | 采调用栈 | 等价 `--call-graph fp` |
| `-e E` | 按事件采 | `-e major-faults -g` = 缺页火焰图 |
| `-p PID` / `-a` | 进程 / 全系统 | 生产限 PID |
| `-C CPU` | 指定核 | 隔离核专项 |
| `-- sleep N` | 限定时长 | **生产铁律** |
| `-o file` | 输出文件 | 默认 perf.data |

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 尾延迟归因 | `-F 499 -g -p PID -- sleep 60`：抓 P99 尖刺的栈——尖刺是稀有事件，**长窗 + 高频**才采得到 |
| 隔离核专项 | `-C 2,3 -a`：只采隔离核，热核视角不被普通核稀释 |
| 缺页火焰图 | `-e major-faults -g`：直接对照 [THP/mmap 决策](../../../06-linux-mm/) |
| perf.data 归档 | 每次调优前后留档——[ch16 baseline 纪律](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md)的可回放要求 |
| ring buffer 设计模式 | perf mmap 环是零拷贝双指针协议的教科书实现——与 io_uring/DPDK ring 对照着读 |
| Pi5 | ARM fp 栈回溯依赖 `-fomit-frame-pointer` 未开；SPE 缺席 → 栈与 IP 精度受限，火焰图仍可用 |

---

## 衔接

- 上一节：[stat 计数机制](./section-13.8-perf-stat-事件计数.md)
- 下一节：[report/script——perf.data 的两种读法与火焰图](./section-13.10-perf-report-与-perf-script.md)
- off-CPU 缺口：[Ch 15 BPF offcputime](../../chapter-15-bpf/)
- 内存事件深入：[Ch 7](../../chapter-07-memory/) · [06-linux-mm](../../../06-linux-mm/)

---

## 代码自测

<details><summary>Q1：一次 perf record 样本从溢出到落盘，经过哪几站？</summary>

PMC 溢出（多为 NMI）→ `__perf_event_overflow`（core.c:9500）→ `irq_work_queue` 推迟（:2471，NMI 上下文不能拿锁）→ `__perf_event_output` → `perf_output_sample` 按 sample_type 序列化 → 写 mmap ring buffer（用户态零拷贝收割）。落 perf.data 是用户态收割后的二次序列化。
</details>

<details><summary>Q2：perf 的 ring buffer 为什么可以无锁？</summary>

单生产者（内核）单消费者（用户态）+ 各写各的指针：内核只 STORE data_head、只 LOAD data_tail；用户态反之。唯一共享可变状态是两个指针（data_head/data_tail），配上 release/acquire 语义的内存屏障（ring_buffer.c:58–98 协议注释）即可——读写同一块环形内存但永远不碰对方正在写的区域。
</details>

<details><summary>Q3：<code>-F 99</code> 为什么比 100 好？-c 和 -F 什么时候选哪个？</summary>

99 避开与 OS timer（100/250/1000Hz）拍频共振——同相位采样会系统性漏采/过采周期任务。-F 适合统计画像（样本量随活动自适应）；-c 适合把采样与工作量对齐（如"每处理 100 万个报文采一次"）。
</details>

<details><summary>Q4：为什么 NMI 里不能直接写 ring buffer，要绕 irq_work？</summary>

NMI 上下文不可睡眠、不可拿自旋锁、甚至部分 per-CPU 基础设施都不可重入。直接做输出可能死锁。irq_work 把工作排队到下一个正常中断上下文执行——代价是时间戳与写入时刻的微小偏差。
</details>

<details><summary>Q5：Release 构建该不该保留帧指针？</summary>

应该：`-g -fno-omit-frame-pointer`。代价约 1-2%（一个寄存器），换来任意时刻可采可读的调用栈。HFT 尾延迟事故里"事后能归因"的价值远超这点稳态开销——没有栈的火焰图只有 [unknown]。
</details>
