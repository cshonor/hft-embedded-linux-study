# 2. 传统工具（16.2）

> 底本：《BPF之巅》第 16 章 虚拟机管理器，16.2 节（印刷 p722–723）

针对虚拟机管理器性能分析和问题诊断的**传统工具不多**。

## 访客系统内

在某些情况下有针对超级调用的跟踪点（见 16.3.1 节的 Xen 跟踪点），除此之外主要依赖通用资源分析工具（前面章节）。

## 宿主机上

- **Xen**：自带工具，包括 `xl top` 和 `xentrace`，可以检测访客系统的资源使用情况
- **KVM**：Linux `perf(1)` 有一个 `kvm` 子命令：

```bash
# perf kvm stat live
11:12:07.687968  Analyze events for all VMs, all VCPUs:

    VM-EXIT    Samples  Samples%  Time%    Min Time  Max Time  Avg time
    HLT        2208     68.90%    99.63%   2.61us    100512.98us  4160.68us
    MSR_WRITE  1668     ...       ...      0.67us    31.74us    3.25us
    PREEMPTION_TIMER  112  ...             4.71us    4638us     ...
    PENDING_INTERRUPT  82  ...             0.92us    ...
    EXTERNAL_INTERRUPT 37  ...             5.36us    84.88us    19.97us
    IO_INSTRUCTION     ...                  3.33us    4.80us    4.07us
    MSR_READ ...  EPT_MISCONFIG ...
    Total Samples: 2421, Total events handled time: 1946040.48us.
```

输出按**虚拟机退出原因**给出统计（样本数、时间占比、最小/最大/平均时间）。示例中最长时间的退出是 **HLT（halt）**——因为虚拟 CPU 进入了空闲状态，这是正常现象。

> 有针对 KVM 事件的跟踪点（包括退出），可以结合 BPF 创建更多详细的工具——这就是 16.4.1 节 kvmexits(8) 的由来。

## 与 BPF 工具的对比

| 维度 | perf kvm stat | kvmexits.bt（BPF） |
|---|---|---|
| 输出形式 | 每原因的 min/avg/max 汇总 | 每原因的**完整延迟直方图**（分布形态可见） |
| 依赖 | perf 采样 | kvm:kvm_exit / kvm_entry 跟踪点 |
| 限制 | 需要 perf 版本支持 | 仅在使用内核 KVM 模块时跟踪点存在 |

## HFT 关联

在交易系统所在 VM 的宿主机（自建 KVM 集群）上，`perf kvm stat live` 是无 BPF 环境下快速判断"VM 为什么退出"的第一步；HLT 占比高通常代表空闲（无害），而 IO_INSTRUCTION / EPT_VIOLATION 频繁则指向设备仿真或内存映射热点。

<details>
<summary>自测题</summary>

1. Xen 在宿主机上提供了哪两个自带的传统分析工具？
2. `perf kvm stat live` 示例中为什么 HLT 的最大时间最长且时间占比最高？
3. BPF 版 kvmexits 相比 perf kvm stat 的主要优势是什么？

<details><summary>参考答案</summary>

1. `xl top`（资源使用概览）与 `xentrace`（事件跟踪）。
2. HLT = 访客 vCPU 空闲时主动 halt 等中断——**空闲本身就是"退出直到下次中断"**，所以单次 halt 可以持续到 100ms 级（等定时器/网络中断唤醒），时间占比自然最高（99.63%）。这是正常现象不是问题；真正要盯的是 IO_INSTRUCTION / EPT_VIOLATION / PREEMPTION_TIMER 这类非空闲退出。
3. **完整延迟直方图**（分布形态可见）vs min/avg/max 三值汇总——多峰/长尾在 avg 里完全不可见，直方图一眼看出（对照 argdist 的均值失明三例）。且 kvmexits 用跟踪点对（kvm_exit/kvm_entry）而非 perf 采样，不受采样偏差影响。
</details>
</details>
