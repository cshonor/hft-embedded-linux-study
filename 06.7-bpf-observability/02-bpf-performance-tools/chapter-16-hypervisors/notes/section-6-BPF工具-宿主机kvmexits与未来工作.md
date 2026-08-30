# 6. BPF 工具：宿主机 kvmexits 与未来工作（16.4）

> 底本：《BPF之巅》第 16 章 虚拟机管理器，16.4 节（印刷 p732–737）

本节的 BPF 工具用于**从宿主机上**做虚拟机性能分析和故障排除。工具来自 BCC/bpftrace 仓库或专为本书创建。作者最早在 2013 年《Systems Performance》中用 DTrace 开发了 kvmexitlantency.d，2019 年 2 月 25 日为本书开发了 bpftrace 版本。

## kvmexits(8)（16.4.1）

`kvmexits(8)` 是一个 bpftrace 工具，**按原因显示访客系统退出时间的分布**，可展示与管理器相关的性能问题以及如何进一步分析。

### 输出示例

```bash
# kvmexits.bt
Attaching 4 probes..
Tracing KVM exits. Ctrl-c to end.

@exit_ns[30, IO_INSTRUCTION]:
    [1K, 2K)      71   |@@@@@               |
    [8K, 16K)    198   |@@@@@@@@@@@@@@@     |
    [16K, 32K)   129   |@@@@@@@@@           |
    [32K, 64K)    94   ...
    [128K, 256K)  12
    ...
    [4M, 8M)       2   # 最多约 8 毫秒

@exit_ns[1, EXTERNAL_INTERRUPT]:
    [256, 512]   281   |
    [512, 1K)    460   |@@@@@@@@@@@@@@@     |
    [1K, 2K)     463   |@@@@@@@@@@@@@@@@    |
    [2K, 4K)     150
    [4K, 8K)     116
    ...

@exit_ns[32, MSR_WRITE]:
    [512, 1K]   5690
    [1K, 2K]    2978   |@@@@@@@@@@@@        |
    [2K, 4K]    2080
    ...

@exit_ns[12, HLT]:
    [16K, 32K)  4167   |@@@@@@@@@@@@        |
    [32K, 64K)  3920
    [64K, 128K) 4467   |@@@@@@@@@@@@@@@     |
    [128K, 256K] 3483
    [256K, 512K] 1764
    [512K, 1M)   922
    [1M, 2M)     113
    ...
    [256M, 512M) 241
    [512M, 1G]    11   # 最长达到约 1 秒

@exit_ns[48, EPT_VIOLATION]:
    [512, 1K]   6160
    [1K, 2K)    6885   |@@@@@@@@@@@@@@@@@@@@|   # 主体
    [2K, 4K)    7686   |@@@@@@@@@@@@@@@@@@@@|
    [4K, 8K)    2220
    [8K, 16K)    582
    ...
```

**解读**：

- 输出按类型显示退出的分布，包括**退出代码号 + 原因字符串**（键为 `[exit_code, reason]`）
- 最长的退出 **HLT（暂停）达 1 秒——正常现象**：这是 CPU 空闲线程
- **IO_INSTRUCTION 最多需要 8 毫秒**——设备仿真的代价
- EPT_VIOLATION 次数最多（6885 次集中在 1K–2K ns）——扩展页表违规，访客访问需管理器介入的内存

### 实现原理

跟踪 `kvm:kvm_exit` 和 `kvm:kvm_entry` **跟踪点对**（exit 记时间戳和原因，entry 结算差值）。注意：这些跟踪点**仅在使用内核 KVM 模块提高性能时**才存在。

### 源代码（bpftrace，节选）

```bash
#!/usr/local/bin/bpftrace

BEGIN
{
    printf("Tracing KVM exits. Ctrl-c to end\n");
}

// from arch/x86/include/uapi/asm/vmx.h:
@exit_reason[0]  = "EXCEPTION_NMI";
@exit_reason[1]  = "EXTERNAL_INTERRUPT";
@exit_reason[2]  = "TRIPLE_FAULT";
@exit_reason[7]  = "PENDING_INTERRUPT";
@exit_reason[8]  = "NMI_WINDOW";
@exit_reason[9]  = "TASK_SWITCH";
@exit_reason[10] = "CPUID";
@exit_reason[12] = "HLT";
@exit_reason[13] = "INVD";
@exit_reason[14] = "INVLPG";
@exit_reason[15] = "RDPMC";
@exit_reason[16] = "RDTSC";
@exit_reason[18] = "VMCALL";
...                                    # 19~37: VMX 指令类/CR_ACCESS/
@exit_reason[28] = "CR_ACCESS";        # DR_ACCESS 等
@exit_reason[30] = "IO_INSTRUCTION";
@exit_reason[31] = "MSR_READ";
@exit_reason[32] = "MSR_WRITE";
@exit_reason[33] = "INVALID_STATE";
@exit_reason[34] = "MSR_LOAD_FAIL";
@exit_reason[36] = "MWAIT_INSTRUCTION";
@exit_reason[37] = "MONITOR_TRAP_FLAG";
@exit_reason[39] = "MONITOR_INSTRUCTION";
@exit_reason[40] = "PAUSE_INSTRUCTION";
@exit_reason[41] = "MCE_DURING_VMENTRY";
@exit_reason[43] = "TPR_BELOW_THRESHOLD";
@exit_reason[44] = "APIC_ACCESS";
@exit_reason[45] = "EOI_INDUCED";
@exit_reason[46] = "GDTR_IDTR";
@exit_reason[47] = "LDTR_TR";
@exit_reason[48] = "EPT_VIOLATION";
@exit_reason[49] = "EPT_MISCONFIG";
@exit_reason[50] = "INVEPT";
@exit_reason[51] = "RDTSCP";
@exit_reason[52] = "PREEMPTION_TIMER";
@exit_reason[53] = "INVVPID";
@exit_reason[54] = "WBINVD";
@exit_reason[55] = "XSETBV";
@exit_reason[56] = "APIC_WRITE";
@exit_reason[57] = "RDRAND";
@exit_reason[58] = "INVPCID";

tracepoint:kvm:kvm_exit
{
    @start[tid] = nsecs;
    @reason[tid] = args->exit_reason;
}

tracepoint:kvm:kvm_entry
/@start[tid]/
{
    $snum = @reason[tid];
    @exit_ns[$snum, @exit_reason[$snum]] = hist(nsecs - @start[tid]);
    delete(@start[tid]);
    delete(@reason[tid]);
}

END
{
    clear(@exit_reason);
    clear(@start);
    clear(@reason);
}
```

**要点**：与 xenhyper 同样的**手写映射表**模式——从 `arch/x86/include/uapi/asm/vmx.h` 抄录退出原因号到名称的转换，键为 `[代号, 名称]` 复合键；按 tid 暂存 start/reason（kvm_exit 与 kvm_entry 之间用 tid 配对）。

### 边界情况

一些 KVM 配置中没有使用内核 KVM 模块，所需的跟踪点不会触发，该工具不能测量退出。这种情况下可以直接使用 **uprobes 对 qemu 进程插桩**来读取退出原因（添加 USDT 探针将是首选）。

## 未来的工作（16.4.2）

对 KVM 和类似管理器来说，**访客系统的 CPU 可以被看作运行的进程**，且这些进程可以被工具看到（包括 top(1)）。由此引出的问题：

- 访客系统正在 CPU 上做什么？可以读到函数和调用栈吗？
- 访客系统为什么要调用 I/O？

宿主机可以对 CPU 上的**指令指针进行采样**，还可以在 I/O 执行时读取（基于虚拟机退出到管理器时）。例如用 bpftrace 显示执行 I/O 指令的指令指针：

```bash
# bpftrace -e 't:kvm:kvm_exit /args->exit_reason == 30/ {
    printf("guest exit instruction pointer: %llx\n", args->guest_rip);
}'
Attaching 1 probe...
guest exit instruction pointer: ffffffff81c9edc9
guest exit instruction pointer: ffffffff81c9ee8b
guest exit instruction pointer: ffffffff81c9edc9
[...]
```

（`exit_reason == 30` 即 IO_INSTRUCTION，`args->guest_rip` 是访客指令指针。）

**难题**：宿主机上缺乏把指令指针转为函数名称的**符号表**，缺乏进程上下文以得知使用的**地址空间**，甚至不知道哪个进程在运行。可能的解决方案已被讨论多年（包括作者上一本书 [Gregg 13b]）：读取 **CR3 寄存器**获得当前页表根目录、尝试找出正在运行的进程、使用访客系统提供的符号表。

**目前，这些问题只能通过来自访客系统自身的插桩来回答。**

## HFT 关联

- 自建 KVM 集群跑交易 VM 时，kvmexits 是宿主机侧核心工具：EPT_VIOLATION 频繁 → 考虑大页（THP/hugetlbfs）减少页表违规；IO_INSTRUCTION 达毫秒级 → 设法用 virtio/SR-IOV 替代设备仿真
- `args->guest_rip` 单行示例展示了宿主机窥视访客执行位置的雏形，但符号化鸿沟意味着**生产诊断仍需访客内 BPF 配合**——跨层排障时两侧工具要同时备好

<details>
<summary>自测题</summary>

1. kvmexits 用哪两个跟踪点配对？为什么需要按 tid 暂存 start 和 reason？
2. HLT 退出最长达 1 秒为什么是正常的？IO_INSTRUCTION 8ms 说明什么？
3. 什么情况下 kvmexits 的跟踪点不触发？替代方案是什么？
4. `args->guest_rip` 单行程序的作用是什么？为什么它无法直接给出函数名？可能的解决思路有哪些？

</details>
