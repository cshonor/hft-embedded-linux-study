## ① 通过打印调试 · `printk()`

#### `printk()`

| 属性 | 说明 |
|------|------|
| **最常用** 内核调试手段 | |
| **几乎随时安全** | 中断上下文、持锁、SMP |
| 局限 | **控制台初始化前** 无输出 → 早期用 **`early_printk()`**（非跨平台） |

```c
printk(KERN_INFO "device probed: %d\n", id);
```

→ **Ch 2** `printk` vs `printf`

#### 日志等级 · Loglevels

| 宏 | 数值 | 场景 |
|----|------|------|
| **`KERN_EMERG`** | 0 | 系统不可用（panic 前） |
| `KERN_ALERT` | 1 | 立即行动（数据损坏风险） |
| `KERN_CRIT` | 2 | 硬件/子系统级故障 |
| `KERN_ERR` | 3 | 错误（驱动 probe 失败等） |
| `KERN_WARNING` | 4 | 警告（默认等级） |
| `KERN_NOTICE` | 5 | 正常但值得注意 |
| `KERN_INFO` | 6 | 信息（启动横幅类） |
| **`KERN_DEBUG`** | 7 | 调试细节 |

| 行为 | 由 **当前控制台日志等级** 决定是否 **输出到物理终端** |
|------|--------------------------------------------------------|
| 调整 | `dmesg -n 3` / `echo 3 > /proc/sys/kernel/printk`——数值 ≥ 7 级制的界 |
| 记住 | **缓冲区永远全收**（dmesg 可看全部）；等级只过滤**控制台**这条慢通道 |

> printk 走两条路：① 写入环形缓冲区（快，总是发生）；② 若消息等级够"响"，同步刷到**串口/显卡控制台**（慢，可能毫秒级）。高频 `KERN_ERR` 会让系统把时间全花在刷控制台上——这是"printk 拖死系统"的主因，而不是写缓冲区本身。

#### 记录缓冲区 · Log Buffer

| 设计 | 说明 |
|------|------|
| **环形缓冲区** | `CONFIG_LOG_BUF_SHIFT` 定大小（单核时代 ~16KB；服务器内核常见 128KB~1MB） |
| 满则 **覆盖最旧** | 内存可控 |
| 中断里 **无阻塞写** | 写入只是一段拷贝 + 移指针 |
| **NMI/嵌套安全** | 现代内核为 NMI 等极端上下文备了 **per-CPU 安全缓冲**——防止"printk 自己死锁在 printk 里"（崩溃打印触发崩溃再打印的递归） |

| 用户态 | 历史 **`klogd`** + **`syslogd`** 读缓冲写文件 — 现多 **journald** |
|--------|---------------------------------------------------------------|

**HFT：** 生产内核 **少 printk 热路径** — 用 **tracepoint/BPF**（→ SysPerf Ch14/15）或 **动态 debug**（`pr_debug` + `/sys/kernel/debug/dynamic_debug/control` 运行时开关，见 [18.8](./section-18.8-探测系统.md)）。`trace_printk()` 写 ftrace per-CPU 缓冲——无控制台路径、无全局锁，是 printk 的**热路径安全替代品**。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** printk 和 printf 的区别？printk 的日志级别有什么用？

<details><summary>答案</summary>

printk 输出到内核环形缓冲区（log_buf），dmesg 查看。printf 输出到 stdout。printk 有日志级别（KERN_EMERG~KERN_DEBUG），控制台只显示 > console_loglevel 的消息。printk 可在中断上下文调用（printf 不行）。但 printk 有锁，高频调用影响性能。HFT 调试用 trace_printk（写入 trace buffer，无锁更快）。

</details>

**Q2.** 为什么 printk 在高频场景下会拖慢系统？

<details><summary>答案</summary>

printk 持有 logbuf_lock 自旋锁 + console_lock。多核同时 printk → 锁竞争 + IPI flush console。每秒万次 printk 可导致系统卡顿。替代：trace_printk（per-CPU ring buffer 无锁）、ftrace（动态探针）、eBPF（可编程追踪）。HFT 调试网卡收包延迟用 trace_printk + ftrace。

</details>

**Q3.** "缓冲区全收、控制台按等级过滤"——为什么要这样分两层，而不是打印时直接决定要不要？

<details><summary>答案</summary>

两条通道的性能差 3 个数量级：写环形缓冲 ~亚微秒（拷贝+移指针）；刷控制台毫秒级（串口 115200 波特每字符 ~87µs）。打印点（可能在持锁/中断里）必须先走快通道**无条件留存证据**；慢通道的取舍（要不要实时显示）交给**事后/旁路**决定——控制台等级可以在系统运行中调整，新调高级别时缓冲里的历史仍在。若打印时就按"当前显示级别"丢弃，等于让一条慢策略挡在快路径上，且调级别前的重要消息永久丢失。

</details>

</details>
---
