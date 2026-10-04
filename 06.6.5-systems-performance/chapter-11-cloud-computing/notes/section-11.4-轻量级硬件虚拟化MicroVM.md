## 11.4 轻量级硬件虚拟化（MicroVM）

> 章节导航：[11.3 操作系统虚拟化/容器](./section-11.3-操作系统虚拟化-容器.md) · 上一篇 ← · 下一篇 [11.5 其他云技术](./section-11.5-其他云技术.md) · [本章导读](../README.md)

**本节讲什么**：MicroVM 的定位（容器速度 + VM 隔离的折中）、Firecracker 的设计取舍、三种隔离技术的四方对比、以及 HFT 场景的选型判断。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | MicroVM = **砍到最小的 KVM** | 去掉传统 BIOS/PCI 模拟，只留 virtio |
| 2 | **125ms 启动 + <5MB 开销** | Lambda/Fargate 的底层答案 |
| 3 | 隔离强度 = VM（**独立内核**） | 强于容器，弱于专用机 |
| 4 | 观测两面性 | Guest 内可用完整 perf/BPF，宿主侧看 hypervisor |
| 5 | HFT：沙箱好用，**热路径仍裸机** | microVM ≠ 裸机性能 |

---

### 一、为什么需要 MicroVM：容器的安全债

容器快但共享内核（[11.3](./section-11.3-操作系统虚拟化-容器.md)）——内核漏洞 = 全体容器沦陷；VM 隔离硬但重（启动秒级、内存开销百 MB 起）。**MicroVM 就是把 VM 瘦身到接近容器的成本**：

```
            隔离强度
               ▲
   传统 VM ●   │
               │
   MicroVM ●   │ ← 容器的成本曲线，VM 的隔离强度
               │
   容器 ●______│
               └──────────────▶ 启动速度/密度
```

### 二、Firecracker 的设计取舍

Firecracker（AWS 开源，Rust 编写）是 MicroVM 的代表——Lambda/Fargate 的底层：

| 设计选择 | 做法 | 效果 |
|---------|------|------|
| **无传统 BIOS/PCI 模拟** | 不模拟 1981 年的 PC | 启动从秒级降到 **~125ms** |
| **最小设备集** | 只有 virtio-net/blk（不含显卡/USB/声卡） | 攻击面 + 代码面都小 |
| **每 vCPU 一线程** | 无复杂的 vCPU 调度层 | 延迟可预测 |
| **精简 KVM 用法** | 只用 KVM 核心能力 | hypervisor 税最小化 |
| **内存开销 <5MB/VM** | 无完整设备模拟的簿记 | 高密度（一台宿主机数千 microVM） |
| **快照** | 内存+设备状态快照，恢复即用 | 冷启动变温启动（毫秒级） |

与 [11.2](./section-11.2-硬件虚拟化Hardware-Virtualization.md) 对比：传统 VM 的启动慢、重，大头在设备模拟和 Guest 内核初始化——Firecracker 的答案不是「优化到极致」，是**直接砍掉问题**（不模拟老硬件、精简 Guest 镜像如 microd 镜像 5MB）。

**快照恢复的巧妙之处**：把「拉起进程+内核初始化+runtime init」整条冷启动链**预执行并冻结**，请求来时恢复快照——FaaS 冷启动问题的正解（配合 [11.5 FaaS](./section-11.5-其他云技术.md)）。

### 三、四方对比：裸机 / 容器 / MicroVM / 传统 VM

| 维度 | 裸机 | 容器 | MicroVM | 传统 VM |
|------|------|------|---------|---------|
| 启动 | 分钟（装机） | 毫秒 | **~125ms** | 秒~分钟 |
| 内存开销 | 0 | ~0 | **<5MB** | 百 MB |
| syscall 路径 | 原生 | **原生** | VM-EXIT 税（小） | VM-EXIT 税 |
| I/O 路径 | 原生 | **原生** | virtio 代理 | virtio 代理 |
| 内核隔离 | —（就是内核） | **无**（共享内核） | 独立 Guest 内核 | 独立 Guest 内核 |
| cache/TLB 干扰 | 只有同机进程 | 共享 | EPT 税 + 共享物理 | 同左 |
| 观测 | 全套 perf/BPF | 全套（按 cgroup 口径） | Guest 内全套 | Guest 内全套 |
| 密度 | 1 | 极高 | 高（数千/机） | 低-中 |

**本质权衡**：从裸机→容器→microVM→VM，**性能下降换隔离上升**。microVM 的定位是「FaaS 密度场景下隔离/成本的最优点」，不是「性能最优点」。

### 四、观测

与 KVM 同构（[11.2 的两层盲区](./section-11.2-硬件虚拟化Hardware-Virtualization.md)）：

| 位置 | 工具 | 说明 |
|------|------|------|
| Guest 内 | perf / BPF / bpftrace 全套 | **完整内核**在 Guest 里——比容器好观测（容器内看宿主机视图的问题不存在） |
| 宿主机 | `kvm_stat`、`perf kvm`、Firecracker metrics（vCPU 数/IO 吞吐） | microVM 的 hypervisor 侧指标 |

云上用 Lambda/Fargate 时宿主侧不可见——只能靠平台日志/Tracing（[11.5](./section-11.5-其他云技术.md)）。

### 五、HFT / 嵌入式选型

| 场景 | 用 MicroVM？ | 理由 |
|------|-------------|------|
| tick/发单热路径 | ❌ 裸机 | VM-EXIT 税 + EPT 税对微秒 SLA 仍是结构性开销 |
| 策略研究沙箱 | ✅ | 跑不可信代码，隔离硬 |
| 回测批处理（多云混跑） | ✅ | 快照+高密度，弹性好 |
| 第三方信号源解析 | ✅ | 外部输入解析隔离（异常输入/漏洞不伤主进程） |
| 嵌入式安全域 | KVM+RT / 车载虚拟化同思路 | 隔离优先于性能的场景 |

**判断口诀**：隔离诉求 > 微秒延迟诉求 → MicroVM 合格；延迟是第一公民 → 裸机没有替代品。

### 衔接

- 上一节：[11.3 容器](./section-11.3-操作系统虚拟化-容器.md)
- 下一节：[11.5 其他云技术](./section-11.5-其他云技术.md)（FaaS/Unikernel）
- 关联：[11.2 硬件虚拟化](./section-11.2-硬件虚拟化Hardware-Virtualization.md)（VM-EXIT/EPT 机制）、[ch12 基准测试](../../chapter-12-benchmarking/)（云上数字的拷问）

---

### 常见陷阱

1. **把 MicroVM 当裸机性能卖**——VM-EXIT/EPT 税比传统 VM 小但仍存在；125ms 启动说的是空 VM，不含应用 init。
2. **快照恢复以为零成本**——恢复要重映射内存、重建设备状态，毫秒级且随内存大小增长。
3. **沙箱内跑延迟基准再外推**——沙箱的 EPT/代理开销叠加在结果里，外推裸机要扣除。

<details>
<summary>自测题（点击展开）</summary>

1. Firecracker 为什么能 125ms 启动？
   <details><summary>答</summary>砍掉传统 BIOS/PCI 模拟（不模拟老 PC）、最小 virtio 设备集、精简 Guest 镜像——问题不是被优化而是被删除。</details>
2. MicroVM 相对容器的核心优势？
   <details><summary>答</summary>独立 Guest 内核——内核级隔离（逃逸面大幅缩小），同时保持接近容器的启动速度与内存开销。</details>
3. 快照如何解决 FaaS 冷启动？
   <details><summary>答</summary>把进程拉起+内核 init+runtime init 整条链预执行并冻结成快照，请求来时恢复——把冷启动变成温启动（毫秒）。</details>
4. HFT 哪些场景适合 MicroVM？
   <details><summary>答</summary>研究沙箱（跑不可信代码）、回测批处理、第三方信号解析——隔离诉求大于微秒延迟诉求的周边。</details>
5. MicroVM 的观测为什么比容器好做？
   <details><summary>答</summary>Guest 有完整内核，perf/BPF 在 Guest 内看的是自己的真况；容器内传统工具显示的是宿主机视图。</details>

</details>


---

← [本章导读](../README.md)
