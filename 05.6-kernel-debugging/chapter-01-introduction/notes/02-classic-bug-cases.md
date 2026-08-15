# 1.2 经典 Bug 案例

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

从历史经典 bug 中学习调试方法论——每个案例都揭示了不同的 bug 类型、检测方法和教训。

## 经典 Bug 案例汇总

| 案例 | 年代 | Bug 类型 | 损失 | 教训 |
|------|------|---------|------|------|
| Patriot 导弹 | 1991 | 定点数累积误差 | 28 人丧生 | 时钟精度需要长期验证 |
| Ariane 5 | 1996 | 64-bit float → 16-bit int 溢出 | 5 亿美元 | 类型转换需要范围检查 |
| Mars Pathfinder | 1997 | 优先级反转 | 任务中断 | 并发设计需要优先级继承 |
| Boeing 737 MAX | 2018 | 单点故障 + 软件设计 | 346 人丧生 | 冗余设计 + 软件架构 |
| Linux VFS 死锁 | 2.6.37 | 锁顺序反转 | 数据损坏 | LOCKDEP 自动检测 |
| Heartbleed | 2014 | 缺少边界检查 | 数据泄露 | 内存安全至关重要 |

## 案例详解

### 1. Patriot 导弹 (1991)

**Bug**：系统时钟以 1/10 秒为单位计数，用 24 位定点数表示。运行 100 小时后，累积误差达到 0.34 秒，导致雷达追踪偏差 600 米，错过来袭导弹。

**内核类似问题**：内核中 `jiffies` 和 `ktime_t` 的精度选择。HFT 系统中时间戳必须用 `ktime_get_ns()`（纳秒精度）而非 `jiffies`（毫秒精度）。

### 2. Ariane 5 (1996)

**Bug**：Ariane 4 的水平速度用 64-bit float 表示，Ariane 5 直接复用代码但速度更快，64-bit float → 16-bit int 转换溢出，导致导航系统异常，火箭自毁。

**内核类似问题**：
```c
// 危险：不检查范围的类型转换
u16 index = (u16)large_value;  // 溢出！
int timeout_ms = jiffies_to_msecs(jiffies);  // 长期运行可能溢出

// 安全：使用范围检查
if (large_value > U16_MAX) {
    pr_err("index overflow: %llu\n", large_value);
    return -EINVAL;
}
u16 index = (u16)large_value;
```

### 3. Mars Pathfinder (1997)

**Bug**：优先级反转——低优先级气象线程持有共享总线锁，高优先级总线管理线程等待该锁，中优先级通信线程抢占低优先级线程，导致高优先级线程间接被长时间阻塞，触发 watchdog 重启。

**解决**：启用优先级继承 (priority inheritance)。

**内核对应**：Linux RT 补丁中的 `rt_mutex` 支持优先级继承。主线内核的 `mutex` 在 CONFIG_RT_MUTEXES=y 时也支持。

```c
// 普通 mutex（无优先级继承）
struct mutex my_lock;
mutex_lock(&my_lock);

// RT mutex（支持优先级继承）
struct rt_mutex my_rt_lock;
rt_mutex_lock(&my_rt_lock);
```

### 4. Linux VFS 死锁 (2.6.37)

**Bug**：两个代码路径以相反顺序获取 `inode->i_mutex` 和 `dentry->d_lock`，形成死锁。人工代码审查无法发现，因为涉及跨子系统调用链。

**教训**：催生了 LOCKDEP 自动锁依赖检测器。LOCKDEP 运行时构建锁依赖图，自动发现潜在死锁。

## Heisenbug 现象

> 加了 printk 就不复现，去掉就复现——因为 printk 改变了时序。

| Heisenbug 类型 | 原因 | 应对策略 |
|---------------|------|---------|
| printk 掩盖竞态 | printk 序列化输出改变时序 | 用 trace_printk 替代 |
| KASAN redzone 掩盖越界 | 额外内存改变了分配布局 | 用 KFENCE 采样检测 |
| 编译器优化差异 | -O0 vs -Og 改变代码布局 | 用 -Og 调试 |
| 调试断点改变时序 | KGDB 单步暂停 CPU | 用 ftrace 无侵入追踪 |

## HFT 关联

- Ariane 5 案例：类型转换溢出在 HFT 交易系统中同样致命——订单数量、价格计算中的整数溢出可能导致错误订单
- Mars Pathfinder：优先级反转是 HFT 系统的典型陷阱——交易线程被低优先级日志线程阻塞
- 教训：边界条件测试和并发设计验证至关重要，HFT 系统应使用 RT 内核 + 优先级继承

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Mars Pathfinder 的优先级反转问题是什么？如何解决的？

> 低优先级气象线程持有共享锁，高优先级总线管理线程等待该锁，中优先级线程抢占低优先级线程导致高优先级线程间接被阻塞。解决方法：启用优先级继承 (priority inheritance)，低优先级线程在持有锁时临时继承等待者的优先级。

**Q2:** 什么是 "Heisenbug"？内核调试中为什么常见？

> Heisenbug 是指在调试模式下消失或行为改变的 bug。内核中常见因为：加调试选项改变内存布局/时序、printk 改变时序导致竞态消失、KASAN 的 redzone 掩盖越界。应对策略：用 ftrace（开销小）替代 printk，用 KCOV 做覆盖率引导模糊测试。

**Q3:** 经典内核 bug 案例（如 Linux 2.6.37 的 VFS 死锁）给调试方法演进带来了什么启示？

> 复杂死锁难以通过代码审查发现，催生了 LOCKDEP 自动锁依赖检测。这类 bug 说明：人工分析锁依赖在大规模代码中不可行，需要运行时自动构建依赖图。

**Q4:** Ariane 5 的溢出 bug 在 HFT 交易系统中可能如何体现？

> 订单数量（如以股为单位）转换为更小单位（如以 0.0001 股为单位的定点数）时可能溢出。价格计算中 64-bit 中间结果转 32-bit 也会溢出。解决方案：始终用足够大的类型（u64/int64），在关键路径做范围检查，用 UBSAN 检测未定义行为。

**Q5:** Patriot 导弹的时钟累积误差与内核时间管理有什么关联？

> 内核 `jiffies` 在 32 位系统上约 49.7 天溢出（HZ=1000）。HFT 系统应使用 `ktime_get_ns()`（64-bit 纳秒，约 584 年才溢出）。长期运行的系统需要注意时间累积误差，定期与 NTP/PTP 同步。

</details>

## 交叉引用

- [05.6 ch08 LOCKDEP](chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch08 KCSAN](chapter-08-lock-debug/notes/05-kcsan.md)
