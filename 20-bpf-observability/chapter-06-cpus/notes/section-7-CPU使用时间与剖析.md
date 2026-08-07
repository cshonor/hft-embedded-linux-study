# 7. CPU 使用时间与剖析 (On-CPU)

### `cpudist`

统计线程每次 **被调度上 CPU 后连续运行多久** 的分布（时间片长度分布）。

```bash
sudo cpudist-bpfcc -p $(pidof myapp) 10
```

与 `runqlat` 互补：一个看 **等 CPU 多久**，一个看 **上 CPU 后跑多久**。

### `cpufreq`

采样 CPU **实际运行频率** — 是否因省电策略降频。

```bash
sudo cpufreq-bpfcc 5
```

**HFT 生产：** 交易机通常 **performance governor** + 关 C-states；若频率掉下去，延迟会莫名变差。

### `profile` — CPU 栈采样 🔴

按固定频率（如 **99Hz**）采样 **全栈**，统计次数 — 生成火焰图的输入。

```bash
sudo profile-bpfcc -F 99 30
sudo profile-bpfcc -F 99 -p $(pidof myapp) 30
```

| 参数 | 说明 |
|------|------|
| `-F` | 采样频率 Hz |
| `-p` | 仅某进程 |
| `-U` | 仅用户态栈 |

**与 `perf record`：** 同属 on-CPU 采样；BCC 版便于与书中其他 BCC 工具一致、脚本化。

### `syscount`（关联）

按 **系统调用类型** 计数 — 回答「CPU 时间是否耗在 syscall 上」。

```bash
sudo syscount-bpfcc -i 1
```


### 常见陷阱

1. **混淆 profile 采样频率和精确度** — 99Hz 采样意味着每 ~10ms 采一次，微秒级事件可能完全采不到；profile 适合毫秒级以上的热点，不适合微秒级延迟分析
2. **只看 on-CPU profile 忽视 off-CPU** — on-CPU profile 只看 CPU 执行时间，如果线程大部分时间在等待（锁/IO/调度），on-CPU profile 看不到问题；需配合 offcputime
3. **忽视采样偏差** — 采样有统计偏差——频繁调用的短函数可能采样命中少，而偶尔调用的长函数命中多；profile 结果是「采样频次」不是「执行时间」

<details>
<summary>📝 自测题（点击展开）</summary>

1. **CPU 剖析（profiling）的基本原理是什么？**

   <details>
   <summary>参考答案</summary>

   按固定频率（如 99Hz）在 CPU 上触发定时器中断，中断时记录当前指令地址（IP）和调用栈。大量采样后，某函数在采样中出现比例高 = 该函数占用了较多 CPU 时间。本质是统计采样，不是精确计时——采样频次反映 CPU 时间分布。

   </details>

2. **on-CPU profile 和 off-CPU 分析有什么区别？什么时候需要结合使用？**

   <details>
   <summary>参考答案</summary>

   on-CPU profile：采样线程在 CPU 上执行时的栈——看「CPU 时间花在哪些函数」。off-CPU 分析：记录线程离开 CPU 时的原因和时长——看「等待时间花在什么上」。需要结合：如果 on-CPU profile 显示 CPU 利用率不高（大量空闲），但延迟仍高，说明问题在 off-CPU（等待 IO/锁/调度），需用 offcputime 钻取。

   </details>

3. **BCC profile 工具的 99Hz 采样对 HFT 延迟分析有什么局限？**

   <details>
   <summary>参考答案</summary>

   99Hz = 每 10ms 采一次，而 HFT 延迟尖刺可能在微秒级。采样可能完全错过短事件——一个持续 5 微秒的调度延迟在 10ms 采样间隔下被命中的概率极低。HFT 延迟分析不应依赖采样，应用事件驱动的 BPF 追踪（如 runqlat 直方图、offcputime 逐事件记录）。

   </details>

</details>

---
