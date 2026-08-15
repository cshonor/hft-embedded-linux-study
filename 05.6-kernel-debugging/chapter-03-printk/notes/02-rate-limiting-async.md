# 3.2 速率限制与异步打印

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

printk 的速率限制防止日志风暴，异步打印避免在中断上下文中长时间阻塞。

## printk 速率限制

### 基本用法

```c
// 方式 1: printk_ratelimit() — 全局速率限制
if (printk_ratelimit())
    pr_err("repeated error: %d\n", err);

// 方式 2: pr_xxx_ratelimited() — 宏简写（推荐）
pr_err_ratelimited("error: %d\n", err);
pr_warn_ratelimited("warning: %d\n", val);
pr_info_ratelimited("info: %d\n", val);

// 方式 3: 自定义速率限制器
static DEFINE_RATELIMIT_STATE(rs, 5 * HZ, 3);  // 5秒3条
if (__ratelimit(&rs))
    pr_err("custom rate limited message\n");
```

### 速率限制参数

```bash
# 查看默认值
cat /proc/sys/kernel/printk_ratelimit       # 间隔秒数，默认 5
cat /proc/sys/kernel/printk_ratelimit_burst  # 突发条数，默认 10

# 修改（运行时）
echo 10 > /proc/sys/kernel/printk_ratelimit       # 10秒间隔
echo 100 > /proc/sys/kernel/printk_ratelimit_burst # 100条突发

# 完全禁用速率限制（确保不丢失错误信息）
echo 0 > /proc/sys/kernel/printk_ratelimit
echo 0 > /proc/sys/kernel/printk_ratelimit_burst
```

### 速率限制变体对比

| 宏 | 作用域 | 默认速率 | 适用场景 |
|------|--------|---------|---------|
| `printk_ratelimit()` | 全局共享 | 5s/10条 | 简单场景 |
| `pr_err_ratelimited()` | 全局共享 | 5s/10条 | 推荐 |
| `dev_err_ratelimited()` | 全局共享 | 5s/10条 | 驱动代码 |
| `DEFINE_RATELIMIT_STATE` | 独立 | 自定义 | 精确控制 |
| `printk_once()` | 全局一次性 | N/A | 启动信息 |
| `printk_ratelimited()` | 全局共享 | 5s/10条 | 旧接口 |

```c
// printk_once: 整个系统生命周期只打印一次
printk_once("This message appears only once\n");

// 每个函数实例只打印一次（static local）
static void handle_error(int err)
{
    static bool warned = false;
    if (!warned) {
        pr_err("first error: %d\n", err);
        warned = true;
    }
}
```

## printk 环形缓冲区

```bash
# 环形缓冲区大小
dmesg | head -1
# 可能显示: [    0.000000] Linux version ...
# 环形缓冲区大小由 CONFIG_LOG_BUF_SHIFT 决定（默认 18 = 256KB）

# 查看实际大小
cat /sys/module/printk/parameters/console_may_schedule

# 增大缓冲区（保留更多历史日志）
scripts/config --set-val LOG_BUF_SHIFT 21  # 2MB
make olddefconfig

# 清空缓冲区
dmesg -C  # 清空
dmesg -c  # 读取并清空

# 持续监控
dmesg -w  # follow 模式
```

### 多 CPU 环形缓冲区

```
6.x printk 环形缓冲区架构:

┌─────────────────────────────────────────────┐
│              Global log buffer              │
│  ┌──────┬──────┬──────┬──────┬──────┐      │
│  │ CPU0 │ CPU1 │ CPU2 │ CPU3 │ ...  │      │
│  │ buf  │ buf  │ buf  │ buf  │      │      │
│  └──────┴──────┴──────┴──────┴──────┘      │
│  per-CPU safe buffer (NMI/context)         │
└─────────────────────────────────────────────┘
```

## 异步打印 (printk offloading)

### 问题：printk 的控制台阻塞

```
printk 输出路径:
printk() → 写入环形缓冲区（快） → 输出到控制台（慢！）
                                    ↓
                              串口 UART: 115200 bps
                              每字符 ~87μs
                              200字符消息 ~17ms
                              期间持有 console_lock
```

### 6.x 线程化 printk

```bash
# 查看是否启用
zcat /proc/config.gz | grep PRINTK
# CONFIG_PRINTK=y
# CONFIG_PRINTK_SAFE_LOG_BUF_SHIFT=13
# CONFIG_PRINTK_NMI=y

# 6.x 引入 printk 线程化
# 当 printk 在中断上下文时，将控制台输出委托给
# 内核线程 console_flush_thread，避免长时间禁用中断

# 查看内核线程
ps aux | grep "printk\|console"
# root  123  0  0  ...  [console_flush_thread]
```

### printk 延迟输出

```c
// printk_deferred: 延迟到安全上下文输出到控制台
// 适用于 NMI / hardirq 中
printk_deferred(KERN_ERR "NMI: stack overflow detected\n");

// 工作原理:
// 1. 写入 per-CPU safe buffer（无锁）
// 2. 延迟到 IRQ 退出时 flush 到全局 logbuf
// 3. 由 console 线程输出到控制台
```

## 串口控制台阻塞分析

```bash
# 查看当前控制台
cat /proc/consoles
# ttyS0  -W- (EC p a)  115200 baud

# 串口波特率与阻塞时间
# 115200 bps: 每字符 87μs，200字符 ~17ms
# 921600 bps: 每字符 11μs，200字符 ~2.2ms
# 1500000 bps: 每字符 6.7μs，200字符 ~1.3ms

# 测量 printk 延迟
echo 1 > /sys/kernel/debug/tracing/events/printk/console/enable
echo function_graph > /sys/kernel/debug/tracing/current_tracer
echo 1 > /sys/kernel/debug/tracing/tracing_on
# 触发 printk 后查看 trace
cat /sys/kernel/debug/tracing/trace
```

## HFT 关联

printk 的控制台输出（尤其是串口）可能阻塞数十毫秒。HFT 系统应：

1. **交易线程路径**：不用 printk（用 trace_printk 替代）
2. **控制台日志级别**：设为 1（仅 EMERG/ALERT）
   ```bash
   echo 1 > /proc/sys/kernel/printk
   ```
3. **禁用串口控制台**：
   ```bash
   # 从 cmdline 移除 console=ttyS0
   # 或运行时禁用
   dmesg -n 0
   ```
4. **错误路径**：用 `pr_err_ratelimited()` 防止日志风暴
5. **大缓冲区**：LOG_BUF_SHIFT=21（2MB），确保不丢失关键日志

```c
// HFT 驱动错误处理中的速率限制
static void hft_handle_error(struct hft_dev *dev, u32 status)
{
    if (status & HFT_ERR_TIMEOUT)
        pr_err_ratelimited("DMA timeout on %s\n", dev_name(dev->dev));

    if (status & HFT_ERR_CRC)
        pr_warn_ratelimited("CRC error on %s (count=%u)\n",
                            dev_name(dev->dev), ++dev->crc_count);

    // 严重错误只打印一次
    if (status & HFT_ERR_FATAL)
        pr_emerg("FATAL error on %s, resetting device\n",
                 dev_name(dev->dev));
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 printk 输出到串口可能阻塞数十毫秒？

> 串口 (UART) 输出是同步的，每个字符通过 I/O 端口逐个发送。115200 波特率下每字符约 87μs，一条 200 字符的 printk 消息需要约 17ms。在此期间 printk 持有自旋锁，阻塞所有 CPU 的 printk。6.x 的 printk 线程化部分缓解了这个问题。

**Q2:** `printk_ratelimit()` 的默认速率是多少？如何修改？

> 默认每 5 秒 10 条消息。可通过 `/proc/sys/kernel/printk_ratelimit`（间隔秒数）和 `/proc/sys/kernel/printk_ratelimit_burst`（条数）修改。HFT 系统可设为 `0 0` 完全禁用速率限制（确保不丢失错误信息），或设为较大值减少输出。

**Q3:** printk_ratelimited() 和 printk_once() 的使用场景分别是什么？

> printk_ratelimited()：限制单位时间内的打印次数，适合循环中可能大量触发的警告。printk_once()：整个系统生命周期只打印一次，适合启动时的一次性信息。HFT 调试中，对高频路径用 ratelimited 避免日志风暴。

**Q4:** printk_deferred() 和普通 printk 有什么区别？

> printk_deferred() 只写入 per-CPU safe buffer，不做控制台输出，延迟到 IRQ 退出时由 console 线程处理。普通 printk 会立即尝试输出到控制台（可能阻塞）。printk_deferred 适用于 NMI/hardirq 上下文，避免长时间持有锁。

**Q5:** 为什么 HFT 系统应该关闭串口控制台输出？

> 串口输出是同步的，115200 bps 下一条 200 字符消息阻塞 ~17ms。HFT 热路径中如果触发 printk，串口阻塞会导致延迟毛刺。解决：控制台级别设为 0/1、移除 console=ttyS0、或用 trace_printk 替代（不输出到控制台）。

</details>

## 交叉引用

- [05.6 ch03 printk 基础](chapter-03-printk/notes/01-printk-basics-loglevel.md)
- [05.6 ch03 trace_printk](chapter-03-printk/notes/05-ftrace-printk.md)
- [05.6 ch09 ftrace](chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
