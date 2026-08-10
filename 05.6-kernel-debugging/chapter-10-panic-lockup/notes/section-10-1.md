# 10.1 Kernel Panic 的触发与处理

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 本节要点

### Panic 触发路径

```c
// 主动触发
panic("fatal error: %d\n", err);

// 被动触发
// 1. Oops + panic_on_oops=1
// 2. BUG_ON() + panic_on_bug (某些配置)
// 3. stack protector 检测到栈溢出
// 4. RCU stall (超过阈值未完成宽限期)
// 5. soft lockup (超过阈值不调度)
// 6. hard lockup (超过阈值不响应中断)
```

### Panic 处理流程

```
panic() 调用
  → 打印崩溃信息
  → 调用 panic_notifier_list 回调
  → 如果配置 kdump: kexec 到 crash kernel
  → 否则: 打印 "Kernel panic - not syncing"
  → 如果 panic_timeout > 0: 延迟后重启
  → 否则: 永久挂起
```

### 配置

```bash
# 自动重启延迟
cat /proc/sys/kernel/panic     # 0=不重启
echo 5 > /proc/sys/kernel/panic # 5秒后重启

# Oops 是否升级为 panic
cat /proc/sys/kernel/panic_on_oops  # 0=不升级
echo 1 > /proc/sys/kernel/panic_on_oops

# panic 时打印所有 CPU 栈
echo 1 > /proc/sys/kernel/panic_print
# 位掩码: 1=所有CPU栈 2=所有CPU寄存器 4=所有CPU TLB 8=原因
```

### HFT 关联

HFT 生产环境: `panic_on_oops=1` + `panic=5` + `panic_print=1` — Oops 后 5 秒自动重启，打印所有 CPU 栈帮助分析。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `panic_on_oops=1` 在 HFT 中为什么重要？

> HFT 内核模块 Oops 后系统状态不确定（可能数据结构损坏）。如果不 panic 继续运行，可能产生错误交易。设为 1 确保系统重启到干净状态，配合 `panic=5` 快速恢复。


**Q:** panic 后内核做了哪些事情？

> (1) 设置 panic 进程名到 panic_msg；(2) 打印 panic 信息和调用栈；(3) 打印所有 CPU 的栈（SysRq+t 或 crash_kexec）；(4) 如果配置了 kdump，kexec 跳转到 crash kernel；(5) 否则进入 panic blink/ reboot（根据 panic_timeout）。

</details>

## 交叉引用

- [05.6 ch10 kdump](chapter-10-panic-lockup/notes/section-10-7.md)
