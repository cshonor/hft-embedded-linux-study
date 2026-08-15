# 9. 中断与其他

### `softirqs` / `hardirqs`

测量处理 **软/硬中断** 的 **时间分布**（不仅是次数）— 网络、块设备高负载时内核态飙高的常见原因。

```bash
sudo hardirqs-bpfcc 5
sudo softirqs-bpfcc 5
```

### `smpcalls`

**SMP 跨核调用 (IPI)** 耗时 — 多核同步、TLB shootdown 等。

```bash
sudo smpcalls-bpfcc
```

### `llcstat`

利用 **硬件 PMC**，在内核汇总 **每进程 LLC 命中/未命中**。

```bash
sudo llcstat-bpfcc 5
```

**注意：** 需 PMU 可用；虚拟化环境可能受限。


### 常见陷阱

1. **忽视软中断对 HFT 的影响** — 软中断（softirq）如 NET_RX 在网络包处理时可能占用 CPU，影响 HFT 策略线程；用 runqlat + irq 查看中断影响
2. **混淆硬件中断和软件中断** — 硬件中断由硬件触发（网卡、定时器），软件中断由内核延迟处理（softirq、tasklet）；两者都可被 BPF 追踪
3. **忽视定时器中断的周期性影响** — 周期性定时器中断（timer tick）每毫秒触发一次调度检查，对 HFT 的微秒级延迟有影响；nohz_full 可以减少

<details>
<summary>📝 自测题（点击展开）</summary>

1. **硬件中断和软件中断有什么区别？BPF 如何追踪？**

   <details>
   <summary>参考答案</summary>

   硬件中断（hardirq）：由硬件触发（网卡 IRQ、定时器、NVMe），在 IRQ 上下文执行，不可睡眠。软件中断（softirq）：内核延迟处理机制（NET_RX 处理网络包、BLOCK 处理 IO），在 softirq 上下文执行。BPF 追踪：`irq:irq_handler_entry`（硬件中断）、`tracepoint:irq:softirq_entry`（软件中断）。

   </details>

2. **定时器中断（timer tick）对 HFT 有什么影响？如何减少？**

   <details>
   <summary>参考答案</summary>

   定时器中断默认每秒 100-1000 次（HZ 配置），每次中断：(1) 触发调度器检查时间片；(2) 更新统计计数器；(3) 可能抢占用户态线程。HFT 微秒级延迟受周期性 tick 干扰。减少方法：`nohz_full=CPUs`（隔离核上减少 tick）、`rcu_nocbs=CPUs`（RCU 回调迁移到其他核）、`irqaffinity=`（IRQ 路由到非隔离核）。

   </details>

3. **如何用 BPF 分析中断对 HFT 线程的影响？**

   <details>
   <summary>参考答案</summary>

   (1) 统计中断频率：`tracepoint:irq:irq_handler_entry { @[handler] = count() }`；(2) 看中断耗时：`tracepoint:irq:irq_handler_entry { @s[handler]=nsecs } tracepoint:irq:irq_handler_exit /@s[handler]/ { @lat[handler]=hist(nsecs-@s[handler]) }`；(3) 看中断与 HFT 线程调度的关系：用 sched_switch 关联中断和线程切换时间。

   </details>

</details>

---
