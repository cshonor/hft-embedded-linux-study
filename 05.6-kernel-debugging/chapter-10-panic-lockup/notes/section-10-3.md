# 10.3 Hard Lockup：CPU 不响应中断

> 🔴 精读

## 本节要点

### Hard Lockup 检测

Hard lockup = CPU 不响应中断超过阈值。比 soft lockup 更严重。

```
Hard lockup 检测:
  NMI (Non-Maskable Interrupt) 定期触发
  检查 hrtimer 是否在预期时间内执行
  如果 hrtimer 没执行 → CPU 不响应中断 → hard lockup
```

### 报告示例

```
[  123.456789] Watchdog detected hard LOCKUP on cpu 2
[  123.456795] CPU: 2 PID: 0 Comm: swapper/2
[  123.456800] Call trace:
[  123.456805]  my_spin_lock_loop+0x200/0x300
```

### 常见原因

| 原因 | 说明 |
|------|------|
| 中断禁用过久 | `local_irq_disable()` 后执行耗时操作 |
| NMI 处理函数死循环 | NMI 处理函数中不应有循环 |
| 硬件问题 | CPU 卡死、总线挂死 |
| 自旋锁 + 中断禁用 | `spin_lock_irq()` 临界区过长 |

### 配置

```bash
# hard lockup 检测
cat /proc/sys/kernel/watchdog_thresh
echo 10 > /proc/sys/kernel/watchdog_thresh  # 阈值

# 禁用 hard lockup 检测 (不推荐)
echo 0 > /proc/sys/kernel/nmi_watchdog
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** hard lockup 和 soft lockup 的区别是什么？

> Soft lockup: CPU 在内核态执行过久未调度，但中断仍能响应（hrtimer 还在更新时间戳）。Hard lockup: CPU 不响应中断（hrtimer 都无法执行），更严重。Soft lockup 可能升级为 hard lockup（如果禁用了中断）。


**Q:** hard lockup 检测为什么需要 NMI？普通中断不行吗？

> hard lockup 的定义就是 CPU 中断被禁用（local_irq_save）。普通中断无法触发。NMI（Non-Maskable Interrupt）不可被禁用，即使中断关闭也能触发。x86 用 PMU 的 NMI 输出，ARM64 用 ARM watchdog（SPMI/WDT）。

</details>

## 交叉引用

- [05.6 ch10 soft lockup](chapter-10-panic-lockup/notes/section-10-2.md)
