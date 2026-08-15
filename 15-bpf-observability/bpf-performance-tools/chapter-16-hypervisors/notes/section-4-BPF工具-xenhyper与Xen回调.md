# 4. BPF 工具：xenhyper 与 Xen 回调（16.3.2–16.3.3）

> 底本：《BPF之巅》第 16 章 虚拟机管理器，16.3.2 / 16.3.3 节（印刷 p727–731）

## xenhyper(8)（16.3.2）

`xenhyper(8)` 是一个通过 `xen:xen_mc_entry` 跟踪点统计超级调用的 bpftrace 工具，输出**超级调用名称对应的调用次数**。只能用于 Xen 虚拟机以半虚拟化模式引导并使用超级调用时。作者于 2019 年 2 月 22 日为本书开发。

### 输出示例

```bash
# xenhyper.bt
Attaching 1 probe...
@[mmu_update]:         44
@[update_va_mapping]:  78
@[mmuext_op]:        6473
@[stack_switch]:    23445
```

stack_switch（上下文切换）最多——23445 次。

### 源代码（bpftrace）

```bash
#!/usr/local/bin/bpftrace

BEGIN
{
    printf("Counting Xen hypercalls (xen_mc_entry): Ctrl-c to end.\n");
}

// needs updating to match your kernel version: xen-hypercalls.h
@name[0]  = "set_trap_table";
@name[1]  = "mmu_update";
@name[2]  = "set_gdt";
@name[3]  = "stack_switch";
@name[4]  = "set_callbacks";
@name[5]  = "fpu_taskswitch";
@name[6]  = "sched_op_compat";
@name[7]  = "dom0_op";
@name[8]  = "set_debugreg";
@name[9]  = "get_debugreg";
@name[10] = "update_descriptor";
@name[11] = "memory_op";
@name[12] = "multicall";
@name[13] = "update_va_mapping";
@name[14] = "set_timer_op";
@name[15] = "event_channel_op_compat";
@name[16] = "xen_version";
@name[17] = "console_io";
@name[19] = "grant_table_op";
@name[20] = "vm_assist";
@name[21] = "update_va_mapping_otherdomain";
@name[22] = "iret";
@name[23] = "vcpu_op";
@name[24] = "set_segment_base";
@name[25] = "mmuext_op";
@name[26] = "acm_op";
@name[27] = "nmi_op";
@name[28] = "sched_op";
@name[29] = "callback_op";
@name[30] = "xenoprof_op";
@name[31] = "event_channel_op";
@name[32] = "physdev_op";
@name[33] = "hvm_op";

tracepoint:xen:xen_mc_entry
{
    @[@name[args->op]] = count();
}

END
{
    clear(@name);
}
```

**实现要点**：

- 用一个基于内核源代码（xen-hypercalls.h）映射的**转换表**在超级调用操作号（`args->op`）和名称之间转换；由于映射随时间变化，需更新以匹配内核版本
- 通过修改 @map 键，可自定义加入导致超级调用的**进程名称**或**用户态调用栈**

## Xen 回调（16.3.3）

有些事件不是访客对管理器发起超级调用，而是 **Xen 调用虚拟机**时触发（例如用于 IRQ 通知）。

### /proc/interrupts 中的 HYP 行

```bash
# grep HYP /proc/interrupts
        CPU0       CPU1    ...  (8 CPU 系统)
HYP:  10156992   9041115  12156816  9976239  7936087  9903434  9713902  8778612   Hypervisor callback interrupts
```

每 CPU 的回调计数。这些也可以用 BPF 跟踪——kprobe 内核函数 `xen_evtchn_do_upcall()`。

### 统计哪个进程被中断（bpftrace）

```
Attaching 1 probe...
@[ps]:             9
@[bash]:          15
@[java]:          71
@[swapper/7]:    100
@[swapper/3]:    110
@[swapper/2]:    130
@[swapper/4]:    131
@[swapper/0]:    164
@[swapper/1]:    192
@[swapper/6]:    207
@[swapper/5]:    248
```

大部分时间 **CPU 空闲线程（swapper/*）** 被 Xen 回调中断——空闲 CPU 也逃不掉回调开销。

### 回调延迟（funclatency）

```bash
# funclatency xen_evtchn_do_upcall
     nsecs                : count    distribution
       ...
      1024 -> 2047        : 131      |*           |
      2048 -> 4095        : 351      |***         |
      4096 -> 8191        : 365      |****        |
      8192 -> 16383       : 602      |******      |   <- 主体
     16384 -> 32767       : 89       ||
     32768 -> 65535       : 13       ||
```

大部分处理时间在 **1 到 32 微秒**之间。跟踪 `xen_evtchn_do_upcall()` 的**子函数**可获得关于中断类型的更多信息。

## HFT 关联

- xenhyper 输出中 `stack_switch` 占绝对多数意味着上下文切换（含调度器迁移）是 PV 实例的超级调用大头——线程数过多的策略进程会直接放大这个数字
- 回调中断 java 71 次 vs swapper 数百次：确认邻居/空闲 CPU 也在承受回调打扰；对延迟敏感线程可结合第 5/6 章的 runqlat、offcputime 评估回调对调度的实际影响

<details>
<summary>自测题</summary>

1. xenhyper 为什么用手写 @name[0..33] 表而不是 ksym？这个表需要什么维护？
2. xen:xen_mc_entry 的哪个参数标识超级调用类型？
3. Xen 回调（callback）与超级调用（hypercall）方向有何不同？对应哪个内核函数？
4. funclatency xen_evtchn_do_upcall 的输出说明回调处理耗时主要分布在什么范围？

</details>
