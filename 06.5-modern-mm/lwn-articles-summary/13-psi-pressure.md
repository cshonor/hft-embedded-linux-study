# PSI (Pressure Stall Information)

> **原文:** [PSI: Pressure stall information](https://lwn.net/Articles/759781/) (LWN, 2018)
> **作者:** Johannes Weiner (Facebook/Meta)
> **内核版本:** 4.20+
> **对标旧书:** 无 (ULK3/LKD3 未涉及)

---

## 核心观点

PSI 是 Facebook 开发的资源压力监控框架，提供 CPU/内存/I/O 的精细压力指标。

### PSI 指标

```
/proc/pressure/cpu    — CPU 压力
/proc/pressure/memory — 内存压力
/proc/pressure/io     — I/O 压力

输出格式:
  some avg10=1.50 avg60=0.85 avg300=0.50 total=12345678
  full avg10=0.80 avg60=0.40 avg300=0.20 total=6789012

  some: 至少一个任务因资源不足而等待
  full: 所有任务都在等待 (CPU 不可用)
  avg10/60/300: 10秒/60秒/300秒滑动平均
  total: 累计微秒数
```

### 压力定义

| 资源 | some | full |
|------|------|------|
| CPU | 有任务等待 CPU 但没在运行 | 所有非 idle 任务都在等待 CPU |
| 内存 | 有任务在等待内存回收 | 所有任务都在等待内存回收 |
| I/O | 有任务在等待 I/O | 所有任务都在等待 I/O |

### PSI 监控 API

```c
// 源码路径: kernel/sched/psi.c
// 用户态可通过 poll() 监控 PSI 阈值

// 示例: 当内存压力 10 秒内超过 50% 时告警
struct pollfd pfd;
pfd.fd = open("/proc/pressure/memory", O_RDWR);
write(pfd.fd, "some 50000 1000000", 20);  // 50% 压力, 10 秒窗口
poll(&pfd, 1, -1);  // 阻塞直到超阈值
```

### 使用场景

```bash
# 监控内存压力
watch -n 1 cat /proc/pressure/memory

# 设置 PSI 触发器
echo "some 50000 1000000" > /proc/pressure/memory  # 50% some, 10s 窗口
# 当触发时, poll() 返回

# systemd 使用 PSI 做资源保护
# Kubernetes 使用 PSI 做 QoS 保障
```

---

## 与旧书差异

| ULK3 / LKD3 | 现代实现 |
|-------------|---------|
| 无 PSI | PSI (4.20+) 提供精细压力指标 |
| 只有 loadavg | PSI per-resource 压力 + 阈值触发 |
| 无用户态监控 | poll() API 支持事件驱动监控 |

---

## HFT 关联

PSI 可用于 HFT 系统监控：(1) 监控 memory pressure 检测是否接近 OOM；(2) 监控 CPU pressure 检测调度延迟；(3) 设置阈值告警在压力升高时自动通知。但 HFT 不应在交易线程中使用 PSI（poll 是阻塞操作）。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** PSI 的 "some" 和 "full" 有什么区别？哪个对 HFT 更有意义？

> "some" = 至少一个任务在等待资源，"full" = 所有任务都在等待。对 HFT 更有意义的是 "some"——即使只有一个交易线程在等待 CPU（被调度延迟），也会影响交易延迟。"full" 意味着系统完全卡死，对 HFT 来说是灾难性事件。

**Q2:** PSI 如何实现低开销？

> PSI 在调度器和内存回收路径中已有的统计点（如 task 队列变化）增量更新压力计数器。开销约 1% CPU。通过 per-CPU 计数器 + 周期性聚合减少锁竞争。相比 loadavg 的全局计算，PSI per-CPU 聚合更高效。

</details>
