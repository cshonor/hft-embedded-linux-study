# 3.1 printk 基础与日志级别

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

printk 是内核最基本、最常用的调试工具。理解其日志级别、缓冲区机制和上下文安全性是内核调试的基础。

## printk 日志级别

| 级别 | 宏 | 数值 | 用途 | 示例场景 |
|------|-----|------|------|---------|
| KERN_EMERG | `pr_emerg()` | 0 | 系统不可用 | 硬件故障、panic |
| KERN_ALERT | `pr_alert()` | 1 | 必须立即处理 | 严重数据损坏 |
| KERN_CRIT | `pr_crit()` | 2 | 严重条件 | 关键设备故障 |
| KERN_ERR | `pr_err()` | 3 | 错误条件 | 驱动初始化失败 |
| KERN_WARNING | `pr_warn()` | 4 | 警告 | 配置异常、降级运行 |
| KERN_NOTICE | `pr_notice()` | 5 | 正常但重要 | 热插拔事件 |
| KERN_INFO | `pr_info()` | 6 | 信息 | 驱动加载成功 |
| KERN_DEBUG | `pr_debug()` | 7 | 调试 | 详细运行状态 |

### pr_xxx 简写宏

```c
// 推荐：使用 pr_xxx 简写（自动添加 KERN_xxx 前缀）
pr_emerg("system halted: %s\n", reason);
pr_alert("critical failure in %s\n", __func__);
pr_crit("hardware fault: reg=0x%x\n", reg);
pr_err("device %s failed: %d\n", dev->name, err);
pr_warn("low memory: %lu KB free\n", free_kb);
pr_notice("hotplug: device %s added\n", dev_name);
pr_info("driver version %s loaded\n", VERSION);
pr_debug("probe called for %s\n", dev_name);  // 受 dyndbg 控制

// 带 KERN_CONT 续行（6.x 推荐用 pr_cont）
printk(KERN_ERR "Error: ");
pr_cont("code=%d\n", err);  // 续在同一行

// 带级别前缀的原始形式（不推荐，用 pr_xxx 替代）
printk(KERN_ERR "device %s failed: %d\n", dev->name, err);
```

## 控制台日志级别

```bash
# 查看/设置控制台日志级别
cat /proc/sys/kernel/printk
# 4 4 1 7
# ^ ^ ^ ^
# | | | └── default (默认日志级别)
# | | └──── minimum (最小日志级别)
# | └────── boot_default (启动默认)
# └──────── current (当前控制台级别)

# 只有级别 < current 的消息会打印到控制台
echo 8 > /proc/sys/kernel/printk  # 打印所有级别
echo 1 > /proc/sys/kernel/printk  # 只打印 EMERG/ALERT
echo 0 > /proc/sys/kernel/printk  # 完全关闭控制台输出

# 临时设置（dmesg 命令）
dmesg -n 8   # 设置控制台级别为 8（显示所有）
dmesg -n 1   # 只显示 EMERG
dmesg -n 0   # 关闭控制台输出
```

## printk 的关键特性

### 中断上下文安全性

```c
// printk 在任何上下文中都安全（包括硬中断、NMI）
irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    // ✅ 安全：printk 使用自旋锁保护环形缓冲区
    pr_err("interrupt triggered: irq=%d\n", irq);

    // ⚠️ 但应尽量简短——长时间持有 lock 影响其他 CPU
    // 大量输出应使用 printk_deferred() 或 trace_printk()

    return IRQ_HANDLED;
}

// printk_deferred: 延迟到非中断上下文输出到控制台
printk_deferred(KERN_ERR "deferred output from NMI\n");
```

### printk vs printf 对比

| 特性 | printk | printf |
|------|--------|--------|
| 上下文 | 任何上下文（含中断、NMI） | 仅进程上下文 |
| 线程安全 | 是（自旋锁保护环形缓冲区） | 是 |
| 中断安全 | 是 | N/A |
| 格式化 | 大部分相同 | 标准 C |
| `%pK` | 内核特有（需 CAP_SYSLOG） | 无 |
| `%pOF` | 设备树节点格式 | 无 |
| 浮点 | ❌ 不支持 | ✅ |
| 控制台输出 | 可能阻塞（串口） | 通常不阻塞 |
| 缓冲区 | per-CPU ring buffer | glibc 缓冲 |

### printk 环形缓冲区

```bash
# 环形缓冲区大小由 CONFIG_LOG_BUF_SHIFT 决定
# 默认 18 = 256KB
zcat /proc/config.gz | grep LOG_BUF_SHIFT
# CONFIG_LOG_BUF_SHIFT=18

# 自定义大小（大缓冲区保留更多历史日志）
scripts/config --set-val LOG_BUF_SHIFT 21  # 2MB
make olddefconfig

# 查看缓冲区使用情况
dmesg | wc -l  # 当前日志行数

# 清空缓冲区
dmesg -C       # 清空不输出
dmesg -c       # 读取并清空

# 持续监控
dmesg -w       # follow 模式（类似 tail -f）
```

### printk 时间戳

```bash
# 查看时间戳格式
dmesg | head -5
# [    0.000000] Linux version ...
# [    1.234567] ...

# 时间戳类型（6.x）
cat /sys/kernel/debug/tracing/trace_clock
# [local] global counter [uptime] [perf] [mono] [mono_raw] [boot]

# 设置时间戳为单调时钟（适合测量相对时间）
echo mono > /sys/kernel/debug/tracing/trace_clock
```

## HFT 关联

HFT 内核模块的 printk 使用策略：

1. **交易热路径**：不使用 printk，用 `pr_debug()` + dyndbg 按需开关
2. **错误路径**：用 `pr_err_ratelimited()` 避免日志风暴
3. **控制台级别**：生产环境设为 1（仅 EMERG/ALERT），避免串口阻塞
4. **初始化/清理**：用 `pr_info()` 记录模块加载/卸载
5. **缓冲区大小**：设为 2MB+（LOG_BUF_SHIFT=21），保留更多历史

```c
// HFT 驱动 printk 使用规范
static int hft_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    pr_info("HFT driver v%s loading on %s\n", VERSION, dev_name(&pdev->dev));

    // 热路径用 pr_debug
    pr_debug("probe: bar0=%pR\n", &pdev->resource[0]);

    // 错误路径用 pr_err_ratelimited
    if (ret)
        pr_err_ratelimited("probe failed: %d\n", ret);

    return ret;
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** printk 在中断上下文中安全吗？为什么？

> 安全。printk 使用自旋锁保护环形缓冲区，且 lockdep 标记为中断上下文安全的锁。但 printk 在中断上下文中应尽量简短（避免大量输出），因为长时间持有锁会影响其他 CPU 的 printk。6.x 引入 lockless printk（printk-safe）缓解此问题。

**Q2:** `pr_debug()` 和 `printk(KERN_DEBUG ...)` 完全等价吗？

> 不完全等价。`pr_debug()` 在未启用 `CONFIG_DYNAMIC_DEBUG` 时仅在 `DEBUG` 宏定义时编译生效。启用 `CONFIG_DYNAMIC_DEBUG` 后，`pr_debug()` 可通过 dyndbg 动态开关。`printk(KERN_DEBUG ...)` 始终编译生效，无法动态关闭。

**Q3:** printk 的日志级别 KERN_DEBUG 和 KERN_INFO 在生产内核中有什么区别？

> KERN_DEBUG（7）默认不输出到控制台（console_loglevel 默认 < 7）。KERN_INFO（6）在大多数配置下输出。但两者都写入环形缓冲区（dmesg 可查），只是控制台输出行为不同。Dynamic Debug 可以运行时启用特定 KERN_DEBUG 消息。

**Q4:** printk 在中断上下文中调用有什么风险？

> printk 获取 logbuf lock（spinlock），如果在中断上下文中调用且中断被持有 lock 的 CPU 触发，可能死锁。6.x 改为 lockless printk（printk-safe）缓解此问题。但在硬中断中仍应避免大量 printk，改用 printk_deferred() 或 trace_printk()。

**Q5:** `%pK` 和 `%p` 在 printk 中有什么区别？为什么 HFT 需要关注？

> `%p` 打印内核地址（任何上下文）。`%pK` 打印内核地址但需要 CAP_SYSLOG 权限——非特权用户看到的是 `(null)`。HFT 系统中 `%p` 可能泄露内核地址（绕过 KASLR），生产环境应使用 `%pK` 或 `%px`（仅 root 可见）。

</details>

## 交叉引用

- [05.6 ch03 速率限制](../../chapter-03-printk/notes/02-rate-limiting-async.md)
- [05.6 ch03 dynamic debug](../../chapter-03-printk/notes/03-dynamic-debug.md)
- [05.6 ch03 trace_printk](../../chapter-03-printk/notes/05-ftrace-printk.md)
