# Hard Lockup：CPU 不响应中断

> 🔴 精读

## 概念详解

### Hard Lockup 是什么

Hard lockup = CPU 不响应中断超过阈值。比 soft lockup 更严重——CPU 完全卡死，连时钟中断都无法处理。

### 检测机制

```
Hard lockup 检测:
  NMI (Non-Maskable Interrupt) 定期触发
  检查 hrtimer 是否在预期时间内执行
  如果 hrtimer 没执行 → CPU 不响应中断 → hard lockup

检测层次:
  NMI watchdog → 检查 hrtimer 是否运行
    → hrtimer 检查 watchdog 时间戳 → 检测 soft lockup
      → watchdog/N 线程 → 检查时间戳

如果 hrtimer 都不运行 → NMI 检测到 hard lockup
```

### 报告示例

```
[  123.456789] Watchdog detected hard LOCKUP on cpu 2
[  123.456795] CPU: 2 PID: 0 Comm: swapper/2
[  123.456800] Call trace:
[  123.456805]  my_spin_lock_loop+0x200/0x300
[  123.456810]  __handle_irq_event_percpu+0x48/0x100
```

### 常见原因

| 原因 | 说明 | 典型场景 |
|------|------|---------|
| 中断禁用过久 | `local_irq_disable()` 后执行耗时操作 | 长临界区 |
| NMI 处理函数死循环 | NMI 处理函数中不应有循环 | NMI handler bug |
| 硬件问题 | CPU 卡死、总线挂死 | 硬件故障 |
| 自旋锁 + 中断禁用 | `spin_lock_irq()` 临界区过长 | 锁内大量计算 |
| PMU 配置错误 | NMI 源配置不当 | watchdog 误报 |

### Soft Lockup vs Hard Lockup

| 特性 | Soft Lockup | Hard Lockup |
|------|------------|-------------|
| 定义 | CPU 未调度超过阈值 | CPU 不响应中断超过阈值 |
| 中断响应 | 仍能响应 | 不响应 |
| 检测器 | watchdog 线程 | NMI watchdog |
| 严重程度 | 警告 | 严重 |
| 可能升级 | 可能升级为 hard lockup | 可能直接 panic |
| 常见原因 | 死循环/锁持有过久 | 中断禁用过久 |

### 配置

```bash
# hard lockup 检测阈值 (与 soft lockup 共用)
cat /proc/sys/kernel/watchdog_thresh
echo 10 > /proc/sys/kernel/watchdog_thresh  # 阈值

# 禁用 hard lockup 检测 (不推荐)
echo 0 > /proc/sys/kernel/nmi_watchdog

# 启用
echo 1 > /proc/sys/kernel/nmi_watchdog
```

### ARM64 上的 Hard Lockup 检测

```
x86:
  NMI 不可屏蔽中断 → PMU 硬件计数器溢出产生 NMI
  NMI handler 检查 hrtimer 是否执行

ARM64:
  使用 ARM watchdog (SPMI/WDT)
  或 PMU 中断 (性能计数器溢出)
  CONFIG_ARM_SDE_INTERFACE=y (SDEI watchdog)
  CONFIG_ARM64_ERRATUM_1024128=y (特定 CPU 修正)
```

### 修复方法

```c
// 错误: 禁用中断后执行耗时操作
void bad_function(void) {
    unsigned long flags;
    local_irq_save(flags);
    // 以下操作耗时过长 → hard lockup
    for (int i = 0; i < 1000000; i++) {
        slow_operation();
    }
    local_irq_restore(flags);
}

// 修复 1: 缩短中断禁用时间
void good_function(void) {
    unsigned long flags;
    // 将不需要中断保护的操作移到外面
    for (int i = 0; i < 1000000; i++) {
        slow_operation();  // 不需要中断禁用
    }
    // 只在必要时禁用中断
    local_irq_save(flags);
    quick_critical_section();
    local_irq_restore(flags);
}

// 修复 2: 用 spin_lock_irqsave 替代手动 IRQ 禁用
void good_function2(void) {
    unsigned long flags;
    spin_lock_irqsave(&my_lock, flags);
    // 临界区保持简短
    quick_operation();
    spin_unlock_irqrestore(&my_lock, flags);
}
```

### HFT 关联应用

```c
// HFT 模块中可能触发 hard lockup 的场景
void on_trade_signal(void) {
    unsigned long flags;
    spin_lock_irqsave(&trade_lock, flags);
    
    // 错误: 在锁内执行网络 I/O
    send_trade_order();  // 可能耗时 → hard lockup
    
    // 正确: 只在锁内更新状态，I/O 移到锁外
    prepare_order(&order);
    spin_unlock_irqrestore(&trade_lock, flags);
    send_trade_order();  // 锁外执行
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** hard lockup 和 soft lockup 的区别是什么？

> Soft lockup: CPU 在内核态执行过久未调度，但中断仍能响应。Hard lockup: CPU 不响应中断（连时钟中断都无法执行），更严重。Soft lockup 可能升级为 hard lockup（如果禁用了中断）。

**Q2:** hard lockup 检测为什么需要 NMI？

> hard lockup 的定义就是 CPU 中断被禁用。普通中断无法触发。NMI（Non-Maskable Interrupt）不可被禁用，即使中断关闭也能触发。x86 用 PMU 的 NMI 输出，ARM64 用 ARM watchdog。

**Q3:** `spin_lock_irq()` 临界区过长为什么会导致 hard lockup？

> `spin_lock_irq()` 在获取锁的同时禁用中断。如果临界区执行时间超过 watchdog 阈值，CPU 在此期间不响应任何中断（包括时钟中断），触发 hard lockup。解决：缩短临界区，将耗时操作移到锁外。

**Q4:** ARM64 上 hard lockup 检测和 x86 有什么不同？

> x86 使用 PMU 硬件计数器溢出产生 NMI。ARM64 没有 x86 意义上的 NMI，使用 ARM watchdog（硬件看门狗）或 SDEI（Software Delegated Exception Interface）。CONFIG_ARM_SDE_INTERFACE 提供 SDEI based watchdog。

**Q5:** HFT 模块如何避免 hard lockup？

> (1) 缩短 `spin_lock_irqsave` 临界区；(2) 将网络 I/O、内存分配等耗时操作移到锁外；(3) 使用 workqueue 延迟执行非关键操作；(4) 避免在中断处理函数中执行耗时操作；(5) staging 环境启用 watchdog 检测。

</details>

## 交叉引用

- [05.6 ch10 Soft Lockup](../../chapter-10-panic-lockup/notes/02-soft-lockup.md)
- [05.6 ch10 Watchdog 机制详解](../../chapter-10-panic-lockup/notes/04-watchdog-mechanism.md)
- [05.6 ch08 并发 Bug 类型](../../chapter-08-lock-debug/notes/01-concurrency-bug-types.md)
