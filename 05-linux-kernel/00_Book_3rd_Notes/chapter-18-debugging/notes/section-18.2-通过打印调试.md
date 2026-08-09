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

| 宏示例 | 严重程度 |
|--------|----------|
| **`KERN_EMERG`** | 最高 — 系统不可用 |
| **`KERN_WARNING`** | 警告 |
| **`KERN_DEBUG`** | 调试 |

| 行为 | 由 **当前控制台日志等级** 决定是否 **输出到物理终端** |

用户态查看：`dmesg` · `/var/log/kern.log`（经 **rsyslog/journald**）

#### 记录缓冲区 · Log Buffer

| 设计 | 说明 |
|------|------|
| **环形缓冲区** | 单核时代常见 **~16KB**（现可更大） |
| 满则 **覆盖最旧** | 内存可控 |
| 中断里 **无阻塞写** | |

| 用户态 | 历史 **`klogd`** + **`syslogd`** 读缓冲写文件 — 现多 **journald** |

**HFT：** 生产内核 **少 printk 热路径** — 用 **tracepoint/BPF**（→ SysPerf Ch14/15）或 **动态 debug**。



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

</details>
---
