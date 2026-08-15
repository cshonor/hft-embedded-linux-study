# Soft Lockup：CPU 长时间不调度

> 🔴 精读

## 概念详解

### Soft Lockup 是什么

Soft lockup = CPU 在内核态执行超过阈值（默认 20 秒）未调度。内核 watchdog 检测到后报告警告。

### Watchdog 检测机制

```
watchdog 机制:
  每 CPU 有一个 hrtimer (高精度定时器)
  每 4 秒触发一次，更新 watchdog 时间戳
  
  另有一个内核线程 (watchdog/N) 检查时间戳
  如果当前 CPU 的时间戳超过 20 秒未更新
  → soft lockup 警告

两层检测:
  hrtimer (时钟中断驱动) → 更新时间戳
  watchdog/N 线程 → 检查时间戳是否过期
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

| 原因 | 说明 | 典型代码 |
|------|------|---------|
| 死循环 | while(1) 或条件永不满足 | `while (!done);` |
| 自旋锁持有过久 | 临界区太长 | `spin_lock(); for(...) {...} spin_unlock();` |
| 长时间禁用抢占 | preempt_disable 后耗时操作 | `preempt_disable(); heavy_work(); preempt_enable();` |
| 大量忙等待 | 无 yield 的轮询 | `while (!ready) { /* nothing */ }` |
| 递归过深 | 栈溢出导致死循环 | 递归函数无终止条件 |

### 配置

```bash
# 检测阈值
cat /proc/sys/kernel/watchdog_thresh  # 默认 10 (soft lockup = 2*10 = 20秒)
echo 5 > /proc/sys/kernel/watchdog_thresh  # 更快检测 (10秒)

# 启用/禁用 soft lockup 检测
echo 1 > /proc/sys/kernel/soft_watchdog  # 启用
echo 0 > /proc/sys/kernel/soft_watchdog  # 禁用

# 设置检测 CPU 范围
echo "0-3" > /proc/sys/kernel/watchdog_cpumask  # 只在 CPU 0-3 上检测
```

### 修复方法

```c
// 错误: 内核态长时间循环
void process_all_data(void) {
    while (has_more_data()) {
        process_one();  // 可能执行很久
    }
    // → soft lockup!
}

// 修复 1: 在循环中定期让出 CPU
void process_all_data(void) {
    while (has_more_data()) {
        process_one();
        cond_resched();  // 检查是否需要调度
    }
}

// 修复 2: 将工作拆分到 workqueue
void process_all_data(void) {
    schedule_work(&process_work);  // 异步处理
}

// 修复 3: 限制每次处理的数量
void process_batch(int max_count) {
    int count = 0;
    while (has_more_data() && count < max_count) {
        process_one();
        count++;
    }
    // 剩余的下次再处理
}
```

### HFT 关联应用

HFT 内核模块可能在高优先级线程中执行长循环（如批量处理行情数据），如果禁用了抢占或持有自旋锁，CPU 无法调度其他任务。

```c
// HFT 常见场景: 批量处理行情数据
void on_market_data_batch(struct market_data *data, int count) {
    spin_lock(&data_lock);
    for (int i = 0; i < count; i++) {
        process_one(&data[i]);  // 可能耗时
        if (i % 100 == 0)
            cond_resched();  // 每 100 条让出一次 CPU
    }
    spin_unlock(&data_lock);
}
```

### watchdog_thresh 与检测时间的关系

```
watchdog_thresh = 10 (默认)
  → hrtimer 每 4 秒触发一次
  → watchdog/N 线程每 10 秒检查一次
  → 如果时间戳 > 2 * 10 = 20 秒未更新 → soft lockup

watchdog_thresh = 5 (更敏感)
  → 如果时间戳 > 10 秒未更新 → soft lockup

watchdog_thresh = 60 (更宽松)
  → 如果时间戳 > 120 秒未更新 → soft lockup
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** soft lockup 为什么在 HFT 内核模块中常见？

> HFT 内核模块可能在高优先级线程中执行长循环（如批量处理行情数据），如果禁用了抢占或持有自旋锁，CPU 无法调度其他任务。解决：在循环中定期调用 `cond_resched()` 或将工作拆分到下半部。

**Q2:** soft lockup 和 hard lockup 的区别？

> soft lockup：CPU 在内核态运行超过 20 秒未让出（schedule），但中断仍能响应。hard lockup：CPU 中断被屏蔽超过阈值，连 NMI 都不响应，更严重。

**Q3:** soft lockup 阈值如何调整？HFT 系统应该设多少？

> `echo 5 > /proc/sys/kernel/watchdog_thresh`（默认 10，soft lockup 阈值 = 2×thresh）。HFT 系统建议保持默认甚至降低到 5——如果交易线程卡在内核态超过 10 秒一定是 bug。

**Q4:** `cond_resched()` 如何避免 soft lockup？

> `cond_resched()` 检查当前是否有更高优先级的任务需要调度。如果有，主动让出 CPU。在长时间循环中定期调用可以避免 soft lockup。注意：在持自旋锁时不能调用 `cond_resched()`（会调度 while atomic）。

**Q5:** HFT 隔离核上为什么可能需要禁用 soft lockup 检测？

> HFT 交易线程用 SCHED_FIFO 绑定在隔离核上，长时间不调度是设计行为。如果 watchdog 仍在该核检测，会误报 soft lockup。通过 `watchdog_cpumask` 排除隔离核。

</details>

## 交叉引用

- [05.6 ch10 Hard Lockup](../../chapter-10-panic-lockup/notes/03-hard-lockup.md)
- [05.6 ch10 Watchdog 机制详解](../../chapter-10-panic-lockup/notes/04-watchdog-mechanism.md)
- [05.6 ch10 Panic 触发与处理](../../chapter-10-panic-lockup/notes/01-panic-causes.md)
