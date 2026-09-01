## ⑦ 探测系统 · Poking and Probing

#### 用 UID 做条件开关

重写核心路径时：

```c
if (current_uid().val != 7777)
    old_fork_path();
else
    new_fork_path();   /* 仅测试用户走新代码 */
```

| 目的 | 新代码 bug **不拖垮全体用户** |

#### 限制打印频率

| 手段 | 说明 |
|------|------|
| **`printk_ratelimit()`** | 限制 **同一消息** 打印速率（默认每 5s 最多 10 条，超了静默并周期性补一句"suppressed N messages"） |
| **`printk_ratelimited()`** | 宏把 ratelimit 检查与 printk 合一——现代代码首选 |
| 发生次数限制 | 静态计数 — **仅前 N 次** `printk`（`once=` 风格 / `printk_once`） |

| 问题 | 高频 ISR 里 `printk` → **控制台洪水** → **系统卡死** |
|------|------------------------------------------------------|

> 频率上限怎么算：控制台刷新 ~毫秒级/条。1 万次/秒的打印 = 10 秒的串口时间——ISR 本身早超时了。**ISR 里原则上零打印**；确需计数用 per-CPU 计数器，退出中断后再汇总输出。

#### 现代「条件开关」的等价物

UID 门控的思想（**代码常驻、按条件激活**）在现代内核有了系统化实现：

| 手段 | 激活方式 | 粒度 |
|------|----------|------|
| **dynamic debug**（`pr_debug`/`dev_dbg`） | `echo 'file mydrv.c +p' > /sys/kernel/debug/dynamic_debug/control` | **逐文件/逐行/逐模块**，运行时 |
| tracepoint（静态埋点） | tracefs 打开对应 event | 逐事件 |
| kprobe/ftrace | 动态插桩任意函数 | 逐函数 |
| eBPF（kprobe 上挂程序） | bpftrace 一行命令 | 任意谓词过滤 |

> 演进逻辑：LKD3rd 时代"改代码加 if"→ 编译期 tracepoint → 运行期 dynamic debug → **任意点动态插桩**（kprobe）→ **插桩上再编程**（eBPF）。UID 门控在今天仍有价值——**业务侧**按账户/标签路由调试路径的思路与它完全同构。

→ **Ch 7** ISR 要快 · **Ch 2** 不要用 `printf` · [18.2 printk](./section-18.2-通过打印调试.md) · [05.5-modern-kernel](../../../05.5-modern-kernel/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 如何用条件 UID 在生产环境安全调试？

<details><summary>答案</summary>

技巧：代码中加 `if (current->uid == DEBUG_UID) printk(...)`。生产环境正常运行不打印，需要调试时用 `setuid DEBUG_UID` 运行测试程序 → 触发调试输出。这样不影响生产流量，且不需要重新编译内核。HFT 可用类似方法在特定测试账户的交易路径上启用 trace。

</details>

**Q2.** `printk_ratelimit()` 的默认节奏是什么？为什么"静默时也要周期性补报一句"？

<details><summary>答案</summary>

默认每 **5 秒**窗口最多 **10** 条（`printk_ratelimit_state` 可调，`/proc/sys/kernel/printk_ratelimit`）；超限后消息被吞。补报"suppressed N messages"是为了两个目的：① 告诉你**还有多少**被压掉了（判断问题是在恶化还是自愈）；② 防止你误以为"不打印=没问题"。没有补报的静默是最危险的调试状态——你以为修好了，其实只是消息被限流了。

</details>

**Q3.** dynamic debug 相比"改代码加 if(uid==...)"有什么本质提升？

<details><summary>答案</summary>

① **零代码改动**——`pr_debug` 常驻源码，不需要为调试重新编译/部署内核；② **粒度到行**——可以只开 `mydrv.c:342` 这一行、或按模块/格式串过滤；③ **运行时热切换**——写 sysfs 即生效，对正在复现的问题当场开火；④ **默认零开销**——未激活时 pr_debug 编译为 no-op（除非 `DEBUG` 宏强开）。UID 门控至今胜在**业务语义**（按账户路由），内核侧条件调试已被 dynamic debug/tracepoint 全面接管。

</details>

**Q4.** 高频 ISR 里为什么"原则上零打印"？per-CPU 计数器的替代方案长什么样？

<details><summary>答案</summary>

打印的成本不在格式化而在**控制台通道**（毫秒级/条）与**全局锁**——ISR 里两者都会放大中断延迟，直接违反"上半部越快越好"（Ch 7）。替代：上半部只做 `this_cpu_inc(counter)`（一条指令，无锁无共享 cache line）；退出中断后由 softirq/定时器把 per-CPU 计数**汇总打印**。需要事件细节时按发生次数抽样（前 N 条打全量，其余计数）。ftrace 侧对应手法是 per-CPU ring buffer——trace_printk 天生就是这条路线的官方实现。

</details>

</details>
---
