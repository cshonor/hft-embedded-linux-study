## 11.2 硬件虚拟化（Hardware Virtualization）

> 章节导航：[11.1 云计算背景与架构](./section-11.1-云计算背景与架构.md) · 上一篇 ← · 下一篇 [11.3 操作系统虚拟化/容器](./section-11.3-操作系统虚拟化-容器.md) · [本章导读](../README.md)

**本节讲什么**：hypervisor 类型与 KVM 架构、虚拟化性能开销的四个机制来源（VM-EXIT / I/O 代理 / 嵌套页表 / steal）、硬件直通与 Nitro 如何逼近裸机、Guest 内外观测的盲区边界。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | VM 的每次敏感操作都是 **VM-EXIT** | 陷入 hypervisor = 微秒级税 |
| 2 | **I/O 代理是最大延迟源** | 虚拟网卡走宿主软件路径 |
| 3 | 嵌套页表让 **TLB 有效容量缩小** | 同 workload 更多 TLB miss |
| 4 | **steal time 是唯一的 Guest 内可见证据** | `%st` 高 = 物理争用 |
| 5 | 直通（SR-IOV）≈ 裸机，但失去 **vMotion/在线迁移** | 隔离与灵活性的权衡 |

---

### 一、Hypervisor 类型

| 类型 | 例子 | 特点 |
|------|------|------|
| **Type 1** | VMware ESXi、Hyper-V、Xen | 裸金属 hypervisor，直接管硬件 |
| **Type 2 / 内核模块** | **KVM**（Linux 内核模块） | 宿主机是普通 Linux，KVM 把它变成 Type 1（CPU 虚拟化指令直接支持） |

每个 **VM** = 完整 Guest OS + 虚拟 vCPU/vNIC/vDisk。KVM 架构里，QEMU 是用户态设备模拟器，KVM 内核模块管 CPU/内存虚拟化——「KVM/QEMU」组合中**性能敏感的归内核，设备模拟在用户态**。

### 二、性能开销的四个机制来源

**① VM-EXIT / VM-ENTRY（CPU 虚拟化税）**

Guest 执行敏感指令（I/O 端口、MMIO、HLT、CPUID、写中断控制器…）触发 VM-EXIT，陷入 KVM 处理后 VM-ENTRY 返回：

```
Guest 用户态 ──(普通指令)──> 直接跑，硬件级速度
Guest 内核态 ──(敏感指令)──> VM-EXIT → KVM 处理 → VM-ENTRY → 返回
                              └──── 每次几 µs 的税 ────┘
```

高频敏感操作（时钟读取、IPI、页表操作）会累积可观开销。硬件辅助（APICv/posted interrupt）已把常见路径做进硬件，但**类别税不会归零**。

**② I/O 代理（软件设备模拟）**

默认虚拟网卡/磁盘是**软件设备**：Guest 驱动写 virtio ring → VM-EXIT → KVM 唤醒宿主机上 QEMU 线程 → QEMU 代 Guest 执行真实 I/O → 注回中断。一次网络收包路径多了**两次上下文切换 + 一次 vCPU/线程调度**——延迟和抖动的主要来源。

**③ 嵌套页表（EPT/NPT）——内存虚拟化税**

Guest 虚拟地址 → Guest 物理地址 → Host 物理地址的两次翻译由硬件 EPT 完成，但代价是**单次 TLB miss 的页表走查翻倍**（或更多）。等效于 TLB 有效容量缩小——大内存、稀疏访问的 workload 受伤最重（TLB miss 每次几百 ns，见 [06-linux-mm TLB](../../../06-linux-mm/chapter-03-page-table-management/)）。

**④ Steal time（物理 CPU 争用）**

宿主机超卖 vCPU，Guest 的 vCPU 等待物理 CPU 的时间在 Guest 内可见为 `%st`：

```
Guest 内 top：  %Cpu(s): 75.0 us,  0.0 sy,  0.0 ni, 12.0 id, 13.0 st
                                                       └───── vCPU 被"偷走" 13%
```

`%st` 高 = 物理 CPU 争用，**不是** Guest 算力不足——加 vCPU 没用。

### 三、硬件直通与 Nitro：逼近裸机

| 技术 | 机制 | 效果 | 代价 |
|------|------|------|------|
| **PCI Passthrough** | 整个设备 VFIO 直通给一个 Guest | 绕过全部软件 I/O 路径 | 设备被独占，不能 vMotion |
| **SR-IOV** | 网卡切多个 VF，每 VF 直通一个 Guest | 直通速度 + 多 Guest 共享 | VF 数量有限，vMotion 受限 |
| **AWS Nitro** | 网络/存储 offload 到专用 Nitro 卡（本地 hypervisor 极薄） | **接近裸机**网络性能 | AWS 私有 |

**SR-IOV 的本质**：把「软件模拟设备 + hypervisor 代理」换成「硬件虚拟化的真实设备」——中断直投 Guest（绕过宿主机 CPU），DMA 直接进 Guest 内存。代价是失去在线迁移的透明性（设备状态不连续了）——**隔离性与运维灵活性二选一**，HFT 选前者。

### 四、资源控制与内存超卖

| 控制 | 机制 | 观测信号 |
|------|------|---------|
| vCPU 数量 | 限算力 | — |
| **Balloon driver** | Guest 内驱动"充气"占用内存再归还宿主机 | Guest 内可用内存**突然下降** |
| 内存 overcommit | 宿主机卖超 | 触发宿主 swap/KSM 压缩 → 延迟尖刺 |
| CPU limit（cgroup） | 宿主侧配额 | Guest 内表现为随机的执行停顿 |

Balloon 的诡异之处：Guest 内看到的是「自己的进程吃了内存」——**Guest 内无法区分 balloon 和真实内存压力**，只能对照宿主机数据。

### 五、观测的两层盲区

| 观测位置 | 工具 | 能看到 | 看不到 |
|----------|------|--------|--------|
| **宿主机** | `kvm_stat`（VM-EXIT 原因分布）、`perf kvm`、`perf record -e kvm:*` | 物理资源真况、VM-EXIT 频率、steal 真值 | Guest 内部逻辑 |
| **Guest 内** | 常规 perf/BPF/[top] | 虚拟资源视图（vCPU、virtio） | 物理邻居、balloon、宿主调度延迟、真实中断路由 |

**排障原则**：延迟尖刺在 Guest 内无法解释（无 CPU、无 I/O、无锁证据）→ **问题在虚拟化层或邻居**——升 ticket 看宿主机，或迁 dedicated host。`kvm_stat` 的 VM-EXIT 原因分布是宿主侧第一手证据：`hlt`/`io_instruction`/`ept_violation` 的计数激增直接指向开销类别。

### HFT / 嵌入式关联

- **云上跑延迟敏感组件的最低配置**：裸金属实例（或 dedicated host）+ SR-IOV/增强网络 + 固定 vCPU 绑定（CPU affinity 在 hypervisor 层）——普通共享实例的 steal 和 I/O 代理抖动对微秒 SLA 是结构性的。
- **「Unexplained Win」的云版教训**（[ch16](../../chapter-16-case-studies/)）：云宿主机更换对 Guest 是不可见的——配置 diff 抓不到，只有同周同时段对比 + PMC 数据才能定罪。
- **嵌入式**：实时虚拟化（KVM + RT 补丁）用于安全域隔离（车载/工控）时，VM-EXIT 延迟抖动是核心指标——和 HFT 的关注点同源。
- **自建 vs 云的延迟账**：共置交易所的裸机是 HFT 的物理刚需，云主要用于弹性周边——这不仅是成本问题，是**光速与隔离**问题。

### 衔接

- 上一节：[11.1 云计算背景与架构](./section-11.1-云计算背景与架构.md)
- 下一节：[11.3 操作系统虚拟化/容器](./section-11.3-操作系统虚拟化-容器.md)（无 hypervisor 层的隔离方式）
- 关联：[06-linux-mm TLB/页表](../../../06-linux-mm/chapter-03-page-table-management/)（EPT 两级翻译的底座）、[ch16 云宿主陷阱](../../chapter-16-case-studies/)

---

### 常见陷阱

1. **%st 高了加 vCPU**——steal 是物理 CPU 争用，加 vCPU 让争用更糟；要换 dedicated/裸金属。
2. **把 virtio 网卡的延迟当选型依据**——软件代理路径的抖动不代表 SR-IOV 后的表现，测试必须同口径。
3. **Guest 内内存下降去找进程泄漏**——先排除 balloon（宿主机侧确认），Guest 内无法区分。
4. **以为直通只是性能优化**——SR-IOV 同时牺牲 vMotion 等运维能力，是架构决策不是调优项。

<details>
<summary>自测题（点击展开）</summary>

1. 虚拟化 I/O 延迟的主要来源？
   <details><summary>答</summary>软件设备代理：Guest 写 virtio ring → VM-EXIT → 唤醒 QEMU 线程代执行 → 注回中断——两次上下文切换 + 调度，延迟与抖动双高。直通/SR-IOV 绕过这条路径。</details>
2. EPT 为什么降低性能？
   <details><summary>答</summary>两次地址翻译让 TLB miss 的页表走查成本翻倍，等效 TLB 容量缩小；稀疏大内存 workload 的 TLB miss 上升最明显。</details>
3. Guest 内什么指标能唯一证明物理 CPU 争用？
   <details><summary>答</summary>steal time（%st）——vCPU 等待物理 CPU 的时间；它在 Guest 内可见，其他 hypervisor 层延迟 Guest 内不可见。</details>
4. SR-IOV 的运维代价是什么？
   <details><summary>答</summary>设备状态直通 Guest 后不连续，在线迁移（vMotion）受限——隔离性能与运维灵活性二选一。</details>
5. Guest 内延迟尖刺排查无果，下一步？
   <details><summary>答</summary>升级到宿主机层：kvm_stat 看 VM-EXIT 原因分布、看邻居负载/steal 真值；或直接迁 dedicated host 验证（对照实验）。</details>

</details>


---

← [本章导读](../README.md)
