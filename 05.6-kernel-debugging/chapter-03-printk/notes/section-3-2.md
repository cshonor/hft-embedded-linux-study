# 3.2 速率限制与异步打印

> 🔴 精读

## 本节要点

### printk 速率限制

```c
// 限制每 10 秒最多 10 条
if (printk_ratelimit())
    pr_err("repeated error: %d\n", err);

// 或使用 ratelimited 变体
pr_err_ratelimited("error: %d\n", err);
pr_warn_ratelimited("warning: %d\n", val);

// 自定义速率
static DEFINE_RATELIMIT_STATE(rs, 5 * HZ, 3);  // 5秒3条
if (__ratelimit(&rs))
    pr_err("custom rate limited message\n");
```

### printk 环形缓冲区

```bash
# 环形缓冲区大小
dmesg | head -1
# 可能显示: [    0.000000] Linux version ...
# 环形缓冲区大小由 CONFIG_LOG_BUF_SHIFT 决定（默认 18 = 256KB）

# 查看实际大小
cat /sys/module/printk/parameters/console_may_schedule

# 清空缓冲区
dmesg -C  # 清空
dmesg -c  # 读取并清空
```

### 异步打印 (printk offloading)

6.x 内核支持将 printk 的控制台输出卸载到内核线程：

```bash
# 查看是否启用
zcat /proc/config.gz | grep PRINTK
# CONFIG_PRINTK=y
# CONFIG_PRINTK_SAFE_LOG_BUF_SHIFT=13

# 6.x 引入 printk 线程化
# 当 printk 在中断上下文时，将输出委托给
# 内核线程 console_flush_thread，避免长时间禁用中断
```

### HFT 关联

printk 的控制台输出（尤其是串口）可能阻塞数十毫秒。HFT 系统应：
1. 交易线程路径不用 printk（用 trace_printk 替代）
2. 控制台日志级别设为 1（仅 EMERG/ALERT）
3. 必要时禁用串口控制台输出

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 printk 输出到串口可能阻塞数十毫秒？

> 串口 (UART) 输出是同步的，每个字符通过 I/O 端口逐个发送。115200 波特率下每字符约 87μs，一条 200 字符的 printk 消息需要约 17ms。在此期间 printk 持有自旋锁，阻塞所有 CPU 的 printk。6.x 的 printk 线程化部分缓解了这个问题。

**Q2:** `printk_ratelimit()` 的默认速率是多少？如何修改？

> 默认每 5 秒 10 条消息。可通过 `/proc/sys/kernel/printk_ratelimit`（间隔秒数）和 `/proc/sys/kernel/printk_ratelimit_burst`（条数）修改。HFT 系统可设为 `0 0` 完全禁用速率限制（确保不丢失错误信息），或设为较大值减少输出。


**Q:** printk_ratelimited() 和 printk_once() 的使用场景分别是什么？

> printk_ratelimited()：限制单位时间内的打印次数，适合循环中可能大量触发的警告。printk_once()：整个系统生命周期只打印一次，适合启动时的一次性信息。HFT 调试中，对高频路径用 ratelimited 避免日志风暴。

</details>

## 交叉引用

- [05.6 ch03 trace_printk](chapter-03-printk/notes/section-3-5.md)
