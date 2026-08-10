# 10.5 Hangcheck Timer

> 🔴 精读

## 本节要点

### Hangcheck Timer

Hangcheck 是独立于 watchdog 的挂死检测机制，使用硬件定时器确保即使内核完全挂死也能触发。

```bash
# 模块参数
modprobe hangcheck-timer
# 或编译进内核

# 参数
echo 60 > /sys/module/hangcheck_timer/parameters/hangcheck_margin  # 容差(秒)
echo 30 > /sys/module/hangcheck_timer/parameters/hangcheck_tick   # 检查间隔(秒)
echo 1 > /sys/module/hangcheck_timer/parameters/hangcheck_reboot  # 挂死时重启
```

### 与 watchdog 的区别

| 特性 | Watchdog | Hangcheck |
|------|---------|-----------|
| 触发方式 | 内核线程 | 硬件定时器 |
| 内核挂死时 | 可能失效 | 仍能触发 |
| 检测对象 | CPU 不调度 | 系统时间不前进 |
| 6.x 状态 | 主流 | 较少使用 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Hangcheck 为什么比 watchdog 更可靠？

> Watchdog 依赖内核线程和 hrtimer，如果内核完全挂死（如死锁导致调度器停止），watchdog 线程无法运行。Hangcheck 使用硬件定时器（独立于内核调度），即使内核完全挂死也能触发中断，确保检测到挂死。


**Q:** hangcheck timer 和 watchdog 的区别是什么？

> watchdog 是内核内建机制，检测 CPU 卡住。hangcheck timer 是外部模块，用于检测系统整体无响应（包括 I/O 卡死等）。HFT 系统可两者都用——watchback 检测 CPU 卡死，hangcheck 检测 I/O 路径卡死。

</details>

## 交叉引用

- [05.6 ch10 watchdog](chapter-10-panic-lockup/notes/section-10-4.md)
