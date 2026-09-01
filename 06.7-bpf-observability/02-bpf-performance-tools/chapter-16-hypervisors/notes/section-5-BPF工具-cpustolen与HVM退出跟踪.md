# 5. BPF 工具：cpustolen 与 HVM 退出跟踪（16.3.4–16.3.5）

> 底本：《BPF之巅》第 16 章 虚拟机管理器，16.3.4 / 16.3.5 节（印刷 p731–732）

## cpustolen(8)（16.3.4）

`cpustolen(8)` 是一个 bpftrace 工具，显示**被盗用 CPU 时间**的分布，并区分短期盗用还是长期盗用。这是对虚拟机来说不可用的 CPU 时间，因为它被其他虚拟机使用了（在某些管理器配置中，可以包括在另外一个域中代表该虚拟机被 I/O 代理消耗的 CPU 时间，所以"盗用"一词会产生误导）。作者于 2019 年 2 月 22 日为本书开发。

### 输出示例

```bash
# cpustolen.bt
Attaching 4 probes..
Tracing stolen CPU time. Ctrl-c to end.
@stolen_us:
[0]         30384  |@@@@@@@@@@@@@@@@@@@@|
[2, 4)          0  ||
[4, 8)         28  ||
[8, 16)        41  ||
```

大部分时间没有发生 CPU 盗用（[0] 桶），盗用比率约 **0.1%**（约 32/30416），有 4 次实际盗用。

### 实现原理

通过 **Xen 和 KVM 版本的 kprobes**（`xen_stealclock()` 和 `kvm_stealclock()`）跟踪 **stealclock 半虚拟化调用**实现：

```bash
#!/usr/local/bin/bpftrace

BEGIN
{
    printf("Tracing stolen CPU time. Ctrl-c to end.\n");
}

kretprobe:xen_stealclock,
kretprobe:kvm_stealclock
{
    if (@last[cpu] > 0) {
        @stolen_us = hist((retval - @last[cpu]) / 1000);
    }
    @last[cpu] = retval;
}

END
{
    clear(@last);
}
```

**要点**：

- kretprobe 取 retval（本次读到的累计 steal 时钟），与 @last[cpu] 的差值即这段时间被盗用的量，`hist()` 分桶
- 该方法在许多**频繁发生的事件**（上下文切换、中断）中被调用，根据工作负载不同开销可能很明显
- 对 Xen 和 KVM 以外的管理器需更新代码：其他管理器可能有类似的 `stealclock()` 函数（满足半虚拟化操作表 pv_ops）
- 有一个更高级的函数 `paravirt_stealclock()`，没有绑定到一种管理器类型、听起来更适合跟踪——**但它不可用于跟踪（可能是内联的）**

## HVM 退出跟踪（16.3.5）

随着访客系统从半虚拟化迁移到硬件虚拟化，我们**失去了插桩超级调用的能力**，但访客系统依然需要**退出到管理器**来访问资源，我们希望能够跟踪这些退出。

### 当前方法

用前面章节的现存工具分析**资源的延迟**，并谨记：延迟的某些部分可能与虚拟机管理相关、**不能直接测量出来**。可以尝试与裸机延迟对比来推断。

### 研究方向：hyperupcalls

一个有趣的研究原型可以揭示访客系统的退出可见性——名为 **hyperupcalls** 的研究技术 [Amit 18]：

- 为访客系统提供一种**安全的方法来请求虚拟机管理器运行小型程序**
- 示例用例包括从访客系统跟踪管理器
- 实现方式：通过虚拟机管理器中的**扩展 BPF 虚拟机**，访客系统编译并运行 BPF 字节码
- 当前没有任何云提供商（可能永远不会）提供此功能，但这是另一个使用 BPF 的有趣项目

## HFT 关联

- cpustolen 的直方图比 top 里的单一 %stole 数字更有价值：**短期盗用（微秒级高频）vs 长期盗用（大块抢占）**对延迟分布的影响完全不同——前者造成尾部抖动，后者直接吃掉整个时间片
- HFT 实例若看到持续非零 steal，应优先考虑换专用主机/裸金属实例，而非调优软件
- hyperupcalls 展示了"BPF 跨越虚拟化边界"的可能性，但目前不可依赖

<details>
<summary>自测题</summary>

1. cpustolen 为什么用 kretprobe 而不是跟踪点？"盗用"在什么配置下会产生误导？
2. 为什么不跟踪 paravirt_stealclock()？
3. cpustolen 的开销风险来自哪里？
4. hyperupcalls 是什么？它如何用 BPF 实现？

<details><summary>参考答案</summary>

1. steal 时钟没有现成跟踪点（pv_ops 半虚拟化操作表走的是函数调用），只能 kretprobe 拿返回值（累计 steal 时钟读数）。误导：某些管理器配置中，"stolen"包括了**替本 VM 代理 I/O 消耗的 CPU**（别的域替你干活的时间也算"盗走"）——看起来被抢，实际是自己在别处花的。
2. paravirt_stealclock() 是更高层封装但**可能是内联的**——内联函数没有独立入口地址，kprobe 挂不上（对照 1.7 动态插桩软肋之二）；所以退而跟踪 xen/kvm 各自的具体实现函数。
3. stealclock 在**上下文切换、中断等高频路径**被调用——事件率 = 调度+中断频率，逐事件 kretprobe 的固定成本乘上去就明显了；负载越忙越贵。
4. hyperupcalls：让访客**安全地请求管理器运行小型程序**的研究原型——访客编译 BPF 字节码，由管理器里的扩展 BPF 虚拟机执行。用例包括从访客侧跟踪管理器行为。当前无云厂商提供（可能永远不会有），是"BPF 跨越虚拟化边界"的概念验证。
</details>
</details>
