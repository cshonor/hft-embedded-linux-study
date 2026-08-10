# 10.2 Soft Lockup：CPU 长时间不调度

> 🔴 精读

## 本节要点

### Soft Lockup 检测

Soft lockup = CPU 在内核态执行超过阈值（默认 20 秒）未调度。

```
watchdog 机制:
  每 CPU 有一个 hrtimer (高精度定时器)
  每 4 秒触发一次，更新 watchdog 时间戳
  
  另有一个内核线程 (watchdog/N) 检查时间戳
  如果当前 CPU 的时间戳超过 20 秒未更新
  → soft lockup 警告
```

### 报告示例

```
[  123.456789] watchdog: BUG: soft lockup - CPU#2 stuck for 22s! [my_app:1234]
[  123.456795] CPU: 2 PID: 1234 Comm: my_app
[  123.456800] Call trace:
[  123.456805]  my_busy_loop+0x100/0x200
[  123.456810]  my_ioctl+0x48/0x100
```

### 常见原因

| 原因 | 说明 |
|------|------|
| 死循环 | while(1) 或条件永不满足的循环 |
| 自旋锁持有过久 | 临界区太长 |
| 长时间禁用抢占 | `preempt_disable()` 后执行耗时操作 |
| 大量忙等待 | 无 yield 的轮询 |

### 配置

```bash
# 检测阈值
cat /proc/sys/kernel/watchdog_thresh  # 默认 10 秒 (2*10=20秒触发)
echo 5 > /proc/sys/kernel/watchdog_thresh  # 更快检测

# 启用/禁用
echo 1 > /proc/sys/kernel/soft_watchdog  # 启用
echo 0 > /proc/sys/kernel/soft_watchdog  # 禁用
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** soft lockup 为什么在 HFT 内核模块中常见？

> HFT 内核模块可能在高优先级线程中执行长循环（如批量处理行情数据），如果禁用了抢占或持有自旋锁，CPU 无法调度其他任务。内核 watchdog 检测到 CPU 长时间未更新时间戳就报告 soft lockup。解决：在循环中定期调用 `cond_resched()` 或将工作拆分到下半部。

</details>
