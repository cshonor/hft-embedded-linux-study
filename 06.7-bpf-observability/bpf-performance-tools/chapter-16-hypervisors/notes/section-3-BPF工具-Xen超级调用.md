# 3. BPF 工具：Xen 超级调用（16.3.1）

> 底本：《BPF之巅》第 16 章 虚拟机管理器，16.3.1 节（印刷 p723–727）

如果访客系统使用**半虚拟化（PV）**并使用超级调用，就可以用现存工具插桩：`funccount(8)`、`trace(8)`、`argdist(8)`、`stackcount(8)`——甚至有 **Xen 跟踪点**可用。测量超级调用的**延迟**需要定制化工具。

## 确认系统是 Xen PV

```bash
# dmesg | grep Hypervisor
[    0.000000] Hypervisor detected: Xen PV
```

## 用 funccount 统计 Xen 跟踪点

```bash
# funccount 't:xen:*'
FUNC                                    COUNT
xen:xen_mmu_flush_tlb_one_user          70
xen:xen_mmu_set_pte                     84
xen:xen_mmu_set_pte_at                  95
xen:xen_mc_callback                     97
xen:xen_mc_extend_args                 194
xen:xen_mmu_write_cr3                  194
xen:xen_mc_entry_alloc                 904
xen:xen_mc_entry                       924
xen:xen_mc_flush                      1175
xen:xen_mc_issue                      1378
xen:xen_mc_batch                      1392
```

**xen_mc* 跟踪点用于多重调用（multicall）**：批处理集中处理的超级调用——

1. `xen:xen_mc_batch` 调用开始（托架 bracket）
2. 每个超级调用对应一次 `xen:xen_mc_entry`
3. `xen:xen_mc_issue` 结束
4. **真正的超级调用只发生在一个刷新操作中**，被 `xen:xen_mc_flush` 跟踪

作为性能优化，有两种"惰性"半虚拟模式可允许多重调用缓冲并在以后刷新：一种用于 MMU 更新，另一种用于上下文切换。如果没有 xen_mc_calls 发生，则 issue 和 flush 为零超级调用。

## 统计超级调用数量（argdist + mcidx）

通过 `xen:xen_mc_flush` 跟踪点及其 `mcidx` 参数可对每次刷新发出的超级调用计数：

```bash
# argdist -t 't:xen:xen_mc_flush():int:args->mcidx' -i 1
[17:41:34]
t:xen:xen_mc_flush():int:args->mcidx
    COUNT   EVENT
        44  args->mcidx = 0
       136  args->mcidx = 1
[17:41:35]
        37  args->mcidx = 0
       133  args->mcidx = 1
```

每秒约 **130 个超级调用**，且没有每批多于一个超级调用的批处理情况（mcidx=0 表示没有超级调用发生）。

## 超级调用的调用栈（stackcount）

```bash
# stackcount -t 'xen:xen_mc_issue'
xen_load_sp0 / switch_to / schedule / ...                    6629
xen_load_tls                                                16448
xen_flush_tlb_single / flush_tlb_page / ... / page_fault    46604
xen_set_pte_at / copy_page_range / copy_process.part.33 /
do_fork / sys_clone / do_syscall_64 / ...                 565901
```

最多的路径揭示：`copy_process`（do_fork）触发的 `xen_set_pte_at` 多重调用达 **565901 次**——fork 时页表复制引发大量 MMU 超级调用。过多的超级调用可能是性能问题，此输出有助于揭示原因。注意：超级调用跟踪的开销取决于其速度，繁忙系统上额外开销会很明显。

## 超级调用延迟（funclatency + kprobe）

真正的超级调用只在 flush 时发生，且**没有针对开始/结束的跟踪点**——改用 kprobe 跟踪 `xen_mc_flush()` 内核函数（它包含真正的超级调用）：

```bash
# funclatency xen_mc_flush
     nsecs                : count    distribution
         0 -> 1          : 0        ||
         2 -> 3          : 15       ||
        ...
       256 -> 511        : 32508    |@@@@@@@@     |
       512 -> 1023       : 80586    |@@@@@@@@@@@@@@@@@@@@|   <- 主体
      1024 -> 2047       : 21022    |@@@@@       |
      2048 -> 4095       : 3519     ||
      4096 -> 8191       : 12825    |@@@         |
      8192 -> 16383      : 7141     |@@          |
     16384 -> 32767      : 158      ||
     32768 -> 65535      : 51       ||
```

主体在 **512–1023 纳秒**——这可能是访客虚拟机管理器性能的重要衡量指标。可编写 BCC 工具记住批处理了哪些超级调用，按操作类型分解等待时间。

### 替代法：CPU 性能剖析

用第 6 章的 CPU 剖析，在 `hypercall_page()`（实际是超级调用函数表）或 `xen_hypercall*()` 函数中查找 CPU 时间。图 16-2 显示以 hypercall_page() 结束的 TCP 接收代码路径。

**陷阱**：这种剖析方法可能产生误导——PV 虚拟机通常无权访问基于 PMC 的性能剖析，默认使用**软件剖析**，而软件剖析**无法在禁用 IRQ 的代码路径（包括超级调用）中采样**（6.2.4 节）。

## Xen HVM：跟踪点不触发

```bash
# dmesg | grep Hypervisor
[    0.000000] Hypervisor detected: Xen HVM
# funccount 't:xen:xen_*'
Tracing 27 functions for "t:xen:xen*"... Hit Ctrl-c to end.
FUNC                                    COUNT
Detaching...   # 全部为 0
```

原因：这些代码路径不再是超级调用，而是由 HVM 管理器捕获和处理的**本机调用**。这使得检查管理器性能变得更困难：必须用前面章节的面向资源的常规工具检查，并记住观察到的延迟 = 资源延迟 + 管理器延迟。

## HFT 关联

- 若行情接收/策略进程跑在 Xen PV 实例上，fork 频繁的策略进程会因 `xen_set_pte_at`（do_fork 路径）产生大量 MMU 超级调用——stackcount 能直接量化
- 512–1023ns 的单次超级调用延迟叠加批处理，是 PV 实例天然抖动源之一；对延迟敏感的部署应选 HVM/裸金属实例（但代价是失去超级调用可见性）

<details>
<summary>自测题</summary>

1. xen_mc_batch / xen_mc_entry / xen_mc_issue / xen_mc_flush 各自对应多重调用机制的哪个环节？真正的超级调用发生在哪里？
2. argdist 统计 mcidx 的输出如何解读"每秒约 130 个超级调用"？
3. 为什么不能用跟踪点测 xen_mc_flush 的延迟？用了什么替代方案？
4. 为什么 PV 虚拟机的 CPU 剖析可能采不到超级调用路径？
5. HVM 模式下 funccount 't:xen:*' 为什么全部为 0？

</details>
