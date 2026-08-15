# Kernel Panic 的触发与处理

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 概念详解

### Panic 是什么

Panic 是内核遇到致命错误时彻底放弃运行。系统停止所有处理，打印 panic 信息后挂起或重启。

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

### Panic 触发原因分类

| 原因 | 说明 | 检测机制 |
|------|------|---------|
| 主动调用 | `panic()` 函数 | 开发者代码 |
| Oops 升级 | `panic_on_oops=1` | Oops 处理路径 |
| 栈溢出 | stack protector | 编译器插入检查 |
| RCU stall | 宽限期超时 | RCU stall detector |
| Soft lockup | CPU 长时间不调度 | watchdog hrtimer |
| Hard lockup | CPU 不响应中断 | NMI watchdog |
| OOM | 内存耗尽无法恢复 | OOM killer |
| 断言失败 | BUG_ON() | 编译时插入 |

### 配置参数

```bash
# 自动重启延迟
cat /proc/sys/kernel/panic     # 0=不重启
echo 5 > /proc/sys/kernel/panic # 5秒后重启

# Oops 是否升级为 panic
cat /proc/sys/kernel/panic_on_oops  # 0=不升级
echo 1 > /proc/sys/kernel/panic_on_oops

# panic 时打印所有 CPU 栈
echo 1 > /proc/sys/kernel/panic_print
# 位掩码:
#   1 = 打印所有 CPU 栈
#   2 = 打印所有 CPU 寄存器
#   4 = 打印所有 CPU TLB
#   8 = 打印 panic 原因
#  组合: echo 15 > /proc/sys/kernel/panic_print (全部)

# panic 时是否打印 ftrace 缓冲区
echo 1 > /proc/sys/kernel/traceoff_on_warning
```

### panic_notifier_list

```c
#include <linux/panic_notifier.h>

// Panic 时内核会依次调用注册的回调
// 可用于: 发送告警、保存状态到 NVRAM、关闭设备

static int my_panic_handler(struct notifier_block *nb,
                            unsigned long action, void *data) {
    // data 是 panic 的格式字符串
    pr_emerg("HFT system going down\n");
    return NOTIFY_DONE;
}
```

### panic_print 位掩码详解

| 值 | 含义 | 输出内容 |
|----|------|---------|
| 0 | 无额外输出 | 仅 panic 信息 |
| 1 | 所有 CPU 栈 | 每个 CPU 的调用栈 |
| 2 | 所有 CPU 寄存器 | 每个 CPU 的寄存器转储 |
| 4 | 所有 CPU TLB | TLB 内容 |
| 8 | panic 原因 | 详细的 panic 原因 |
| 15 | 全部 | 以上全部 |

### HFT 关联应用

HFT 生产环境配置：

```bash
# /etc/sysctl.d/99-hft.conf
kernel.panic_on_oops = 1       # Oops 升级为 panic
kernel.panic = 5               # 5 秒后自动重启
kernel.panic_print = 1         # 打印所有 CPU 栈

# 理由:
# 1. Oops 后状态不确定，不能继续交易
# 2. 5 秒延迟足够 kdump 保存 vmcore
# 3. 打印所有 CPU 栈帮助分析崩溃时的全局状态
```

### Panic 日志示例

```
[  123.456789] Kernel panic - not syncing: Fatal exception in interrupt
[  123.456790] CPU: 2 PID: 0 Comm: swapper/2 Tainted: G      W    6.1.63 #1
[  123.456795] Hardware name: Raspberry Pi 5 Model B (DT)
[  123.456800] Call trace:
[  123.456805]  my_irq_handler+0x3c/0x100 [my_module]
[  123.456810]  __handle_irq_event_percpu+0x48/0x100
[  123.456815]  handle_irq_event_percpu+0x1c/0x60
[  123.456820]  ...
[  123.456825] Kernel Offset: disabled
[  123.456830] ---[ end Kernel panic - not syncing: Fatal exception in interrupt ]---
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `panic_on_oops=1` 在 HFT 中为什么重要？

> HFT 内核模块 Oops 后系统状态不确定（可能数据结构损坏）。如果不 panic 继续运行，可能产生错误交易。设为 1 确保系统重启到干净状态，配合 `panic=5` 快速恢复。

**Q2:** panic 后内核做了哪些事情？

> (1) 设置 panic 信息；(2) 打印 panic 信息和调用栈；(3) 打印所有 CPU 的栈（如果 panic_print 配置）；(4) 如果配置了 kdump，kexec 跳转到 crash kernel；(5) 否则根据 panic_timeout 决定重启或挂起。

**Q3:** `panic_print=1` 和 `panic_print=15` 的区别？

> `1` 只打印所有 CPU 的栈。`15` = 1+2+4+8，打印所有 CPU 栈 + 寄存器 + TLB + panic 原因。调试时建议用 15 获取最多信息，但输出量大可能淹没串口缓冲区。

**Q4:** RCU stall 导致的 panic 和 Oops 导致的 panic 有什么区别？

> RCU stall 是 RCU 宽限期超时（某个 CPU 长时间不让 RCU 完成），内核主动调用 panic。Oops 导致的 panic 是代码错误（如 NULL deref）触发 Oops，然后因 panic_on_oops=1 升级。两者 root cause 不同：RCU stall 通常是 CPU 卡住，Oops 通常是代码 bug。

**Q5:** HFT 生产环境为什么不设 `panic=0`（不重启）？

> `panic=0` 表示系统挂死等待人工干预。HFT 要求快速恢复——交易中断每分钟损失大。设 `panic=5` 确保 5 秒后自动重启，比人工干预快得多。5 秒足够 kdump 保存 vmcore（如果配置了）。

</details>

## 交叉引用

- [05.6 ch07 Oops vs Panic](../../chapter-07-oops/notes/01-oops-vs-panic.md)
- [05.6 ch10 Soft Lockup](../../chapter-10-panic-lockup/notes/02-soft-lockup.md)
- [05.6 ch10 Kdump/Kexec](../../chapter-10-panic-lockup/notes/07-kdump-kexec.md)
