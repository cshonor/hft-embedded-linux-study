# 6. 调度器与饱和度

### `runqlat` — 运行队列延迟 🔴

测量：**线程进入 RUNNABLE → 实际在 CPU 上运行** 的等待时间分布。

```bash
sudo runqlat-bpfcc 10          # 每 10s 打印直方图
sudo runqlat-bpfcc -P 1 10     # 仅 CPU 1（绑核核对）
```

| 解读 | 含义 |
|------|------|
| 直方图右尾拉长 | CPU **饱和** — 就绪线程排队 |
| 绑核 dedicated 核接近 0 | 健康 |
| 突刺与行情峰值对齐 | 可能争抢同核、或邻居进程干扰 |

**Gregg 观点：** 排队 **时间 (latency)** 比排队 **长度 (length)** 更直接反映性能影响 — 但仍可用 `runqlen` 辅助。

### `runqlen`

**采样** 各 CPU 运行队列 **长度**（有多少线程在等）。

```bash
sudo runqlen-bpfcc 5
```

### `runqslower`

仅打印 **等待超过阈值** 的线程（如 >10ms）— 适合抓 **长尾**，避免海量输出。

```bash
sudo runqslower-bpfcc 10       # 10ms
```

**HFT runbook：** incident 时 **先 `runqlat` 10s** → 若右尾异常再 `runqslower` 抓具体 PID/栈。


### 常见陷阱

1. **把 run queue 长度和调度延迟混淆** — run queue 长度是瞬时快照（r 列），调度延迟是线程从 ready 到 running 的等待时间（runqlat 测量）；长度不等于延迟
2. **忽视偶发调度延迟尖峰** — 平均调度延迟可能正常，但偶发的毫秒级尖峰足以导致 HFT 策略超时；runqlat 直方图能看到尾部分布
3. **在隔离核上仍看到调度事件** — isolcpus 防止其他线程迁移到隔离核，但不阻止内核线程/定时器中断；需配合 nohz_full 和 irqaffinity 完全隔离

<details>
<summary>📝 自测题（点击展开）</summary>

1. **CPU 饱和度如何测量？runqlat 和 mpstat 的 r 列有什么区别？**

   <details>
   <summary>参考答案</summary>

   mpstat/vmstat 的 r 列是 run queue 长度的瞬时快照（采样时刻有多少线程在等 CPU）。runqlat 测量的是调度延迟——线程从变为 ready 到实际被调度运行的时间分布（直方图）。区别：r 列是「积压量」，runqlat 是「等待时间」。HFT 更关心 runqlat，因为延迟尖刺取决于等待时间而非队列长度。

   </details>

2. **为什么平均调度延迟正常但 HFT 仍有超时？**

   <details>
   <summary>参考答案</summary>

   调度延迟分布可能有长尾——平均值 10 微秒但 99.9 分位是 500 微秒。HFT 策略的超时阈值通常是固定的（如 100 微秒），一次尾部尖峰就足以触发超时。runqlat 直方图能看到尾部分布，平均值正常不代表尾部正常。

   </details>

3. **isolcpus 隔离后仍看到调度事件，可能的原因是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 内核线程（kworker/ksoftirqd）仍在隔离核上运行——需 `rcu_nocbs=` 和 `nohz_full=`；(2) 定时器中断（timer tick）仍触发——需 `nohz_full=`；(3) 硬件中断（网卡 IRQ）路由到隔离核——需 `irqaffinity=` 设置 IRQ 亲和性；(4) isolcpus 只是阻止用户线程迁移，不阻止内核机制。完整隔离需 isolcpus + nohz_full + rcu_nocbs + irqaffinity 组合。

   </details>

</details>

---
