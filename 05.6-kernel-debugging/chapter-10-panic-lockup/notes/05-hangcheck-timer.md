# Hangcheck Timer

> 🔴 精读

## 概念详解

### Hangcheck 是什么

Hangcheck 是独立于 watchdog 的挂死检测机制，使用硬件定时器确保即使内核完全挂死也能触发检测。

### 工作原理

```
Hangcheck Timer:
  使用硬件定时器 (独立于内核调度)
  定期检查系统时间是否前进
  如果系统时间停止前进超过阈值 → 触发重启

参数:
  hangcheck_tick    — 检查间隔 (默认 30 秒)
  hangcheck_margin  — 容差时间 (默认 60 秒)
  hangcheck_reboot  — 挂死时是否重启 (1=重启)
  
  实际超时 = hangcheck_tick + hangcheck_margin
           = 30 + 60 = 90 秒
```

### 配置

```bash
# 加载模块
modprobe hangcheck-timer

# 参数
echo 60 > /sys/module/hangcheck_timer/parameters/hangcheck_margin  # 容差(秒)
echo 30 > /sys/module/hangcheck_timer/parameters/hangcheck_tick   # 检查间隔(秒)
echo 1 > /sys/module/hangcheck_timer/parameters/hangcheck_reboot  # 挂死时重启
```

### 与 Watchdog 的区别

| 特性 | Watchdog | Hangcheck |
|------|---------|-----------|
| 触发方式 | 内核线程 + hrtimer | 硬件定时器 |
| 内核挂死时 | 可能失效 | 仍能触发 |
| 检测对象 | CPU 不调度 | 系统时间不前进 |
| 软件依赖 | 依赖调度器 | 独立于调度器 |
| 6.x 状态 | 主流 | 较少使用 |
| 检测精度 | 较高（4秒粒度） | 较低（30秒粒度） |

### 使用场景

```bash
# 场景 1: 补充 watchdog 的盲区
# watchdog 可能因为调度器停止而失效
# hangcheck 用硬件定时器，不受调度器影响

# 场景 2: 检测 I/O 路径挂死
# 某些 I/O 挂死不会导致 CPU 卡住
# 但系统时间会停止前进

# 场景 3: 双重保障
# HFT 生产环境同时使用 watchdog 和 hangcheck
# watchdog 检测 CPU 卡死 (快速, 20秒)
# hangcheck 检测整体挂死 (慢速, 90秒)
```

### HFT 关联应用

```bash
# HFT 生产环境配置
# watchdog (快速检测) + hangcheck (兜底保障)

# /etc/modules-load.d/hangcheck.conf
hangcheck-timer

# /etc/modprobe.d/hangcheck.conf
options hangcheck-timer hangcheck_tick=30 hangcheck_margin=60 hangcheck_reboot=1

# 检测时间线:
# 0-20s:   watchdog 检测 CPU soft lockup
# 0-10s:   watchdog 检测 CPU hard lockup
# 0-90s:   hangcheck 检测系统整体挂死
# 如果 watchdog 失效, hangcheck 90 秒后重启
```

### 硬件看门狗 vs 软件看门狗

| 类型 | 代表 | 可靠性 | 响应速度 |
|------|------|--------|---------|
| 硬件 WDT | 外部看门狗芯片 | 最高 | 可调（亚秒级） |
| NMI watchdog | PMU/ARM WDT | 高 | 10 秒级 |
| Soft watchdog | hrtimer + 线程 | 中 | 20 秒级 |
| Hangcheck | 硬件定时器 | 高 | 90 秒级 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Hangcheck 为什么比 watchdog 更可靠？

> Watchdog 依赖内核线程和 hrtimer，如果内核完全挂死（如死锁导致调度器停止），watchdog 线程无法运行。Hangcheck 使用硬件定时器（独立于内核调度），即使内核完全挂死也能触发中断。

**Q2:** hangcheck timer 和 watchdog 的区别是什么？

> watchdog 是内核内建机制，检测 CPU 卡住。hangcheck timer 是外部模块，用于检测系统整体无响应（包括 I/O 卡死等）。HFT 系统可两者都用——watchdog 检测 CPU 卡死，hangcheck 检测 I/O 路径卡死。

**Q3:** hangcheck 的实际超时时间如何计算？

> 实际超时 = hangcheck_tick + hangcheck_margin。默认 30 + 60 = 90 秒。tick 是检查间隔，margin 是允许的延迟（系统忙时时间戳可能延迟更新）。如果 90 秒内系统时间没有前进，判定为挂死。

**Q4:** HFT 系统应该用 hangcheck 还是外部硬件看门狗？

> 理想情况下两者都用。hangcheck 是内核模块，不需要额外硬件，但依赖硬件定时器（可能受硬件故障影响）。外部硬件看门狗最可靠（独立于 CPU 和内核），但需要额外硬件。HFT 生产环境建议：watchdog + hangcheck + 外部 WDT 三重保障。

**Q5:** 为什么 6.x 内核中 hangcheck 较少使用？

> 6.x 内核的 watchdog 机制已经比较完善（NMI watchdog + soft watchdog），覆盖了大部分场景。hangcheck 的功能部分被 hw watchdog 子系统取代。但在某些特殊场景（如 I/O 挂死检测）hangcheck 仍有价值。

</details>

## 交叉引用

- [05.6 ch10 Watchdog 机制详解](../../chapter-10-panic-lockup/notes/04-watchdog-mechanism.md)
- [05.6 ch10 Soft Lockup](../../chapter-10-panic-lockup/notes/02-soft-lockup.md)
- [05.6 ch10 Panic 触发与处理](../../chapter-10-panic-lockup/notes/01-panic-causes.md)
