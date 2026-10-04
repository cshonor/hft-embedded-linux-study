## 3.3–3.4 内核演进与 Linux 特性

> [3.2 内核基础](./section-3.2-内核基础与核心概念.md) · [3.1 核心术语](./section-3.1-核心术语.md) · [3.5 其他系统模型](./section-3.5-其他系统模型.md) · [3.6 内核比较](./section-3.6-内核比较.md)

---

### 本节讲什么

两件事：一是 Unix→BSD→Solaris 这条血缘线给现代 Linux 留下了什么（很多「Linux 的东西」其实是继承来的）；二是 Linux 自己的六个性能里程碑 + 三个「现代焦点」（systemd、KPTI、eBPF）。看完能回答：**为什么今天这台跑 HFT 的 Linux 长成这样**。

### 要点

| # | 要点 | 一句话展开 |
|---|------|-----------|
| 1 | Linux 的核心设施大量来自 Unix 血统 | VFS/slab/DTrace 思想来自 Solaris；TCP/IP 栈思路来自 BSD |
| 2 | 六个里程碑是性能分析的「分层地图」 | 调度/同步/IO多路复用/隔离/TLB/虚拟化各占一层 |
| 3 | epoll 是「相对 select/poll」的胜利 | O(1) 就绪事件 vs O(n) 全量扫描——并发模型的地基 |
| 4 | THP 是双刃剑 | 省 TLB miss 的同时引入分配/合并延迟尖刺 |
| 5 | KPTI 给 syscall 加了「Meltdown 税」 | 0.1%~6%，syscall 密集型负载在谱高端 |
| 6 | eBPF 是「观测主线」的地基 | Ch4 工具、Ch15 深挖、06.7 专书，全在这条线上 |

---

### 一、Unix 血统：现代性能设施从哪来

| lineage | 留给现代 Linux 的遗产 | 性能分析中的落点 |
|---------|----------------------|------------------|
| **Unix** | 进程模型、syscall 接口、权限模型 | 3.1 的全部术语在这定义 |
| **BSD** | 按需分页（demand paging）、高性能 TCP/IP 栈思路 | ch7 内存管理、ch10 网络栈的祖先 |
| **Solaris** | **VFS**、**Slab 分配器**、**DTrace**、ZFS | VFS→ch8 文件系统抽象层；slab→内核对象缓存；DTrace→eBPF 的思想先祖；ZFS→ch8 存储选项 |

**要点：** 今天 Linux 里的 VFS、slab、观测文化，很多来自这条演进线——不是「Linux 独有发明一切」。对性能工程师的实际意义：**跨 Unix 血统的概念可以迁移**——在 Solaris 上学会的 DTrace 思维（探针、聚合、动作语言）原封不动适用于 bpftrace；Solaris 上首创的 mdb/kstat 方法论在 /proc + eBPF 上有对应物。Gregg 本人正是从 Solaris 时代（DTrace 作者团队背景）迁移到 Linux 后推动了 BCC/bpftrace——**工具换了，方法论是连续的**。

---

### 二、Linux 性能相关里程碑

| 引入 | 作用 | 深入 |
|------|------|------|
| **O(1) / CFS 调度器** | 可扩展调度；CFS 用红黑树按 vruntime 公平排序 | [LKD Ch 4 调度](../../../05-linux-kernel/chapter-04-process-scheduling/)；CFS 内部结构（rb_root_cached）在 06.7/TLPI 笔记中展开 |
| **RCU** | 读多写少路径低争用同步——读侧几乎零开销 | 内核数据结构章；quiescent state 的思想 |
| **epoll** | 高并发 I/O 多路复用（相对 select/poll） | 下文展开 |
| **cgroups** | 资源隔离与限额 | [11.3 容器](../../chapter-11-cloud-computing/notes/section-11.3-操作系统虚拟化-容器.md) 的 cpu.max 周期冻结机制 |
| **THP（透明大页）** | 2MB 页减少 TLB miss，亦有延迟抖动争议 | [Ch 7 内存](../../chapter-07-memory/)；分配/compaction 尖刺 |
| **KVM** | 硬件虚拟化（共置 vs 云） | [11.2 硬件虚拟化](../../chapter-11-cloud-computing/notes/section-11.2-硬件虚拟化Hardware-Virtualization.md) 四税机制 |

**epoll 为什么值得单独一提**——select/poll 每次调用都要把整个 fd 集合从用户态拷进内核并线性扫描一遍，O(n) 且与 idle 连接数成正比；epoll 在内核里维护就绪队列，`epoll_wait` 只返回就绪的 fd，O(1)。万级并发连接的市场行情网关，select 与 epoll 的差距不是常数倍而是随连接数线性恶化的差距。HFT 的行情分发、撮合网关基本都是 epoll（或更激进的旁路）——这是并发模型的地基性选择。

**THP 双刃剑展开**：收益——2MB 页使 TLB 覆盖范围 ×512，热路径遍历大数组的 TLB miss 显著下降；代价——THP 的分配可能触发内存 compaction（整理碎片），**一个毫秒级的停顿**可能出现在最不该出现的时刻。HFT 常见策略：**关闭 THP，改用显式 hugetlbfs 预留**——把大页的收益留下，把「运行时惊喜」去掉。见 [Ch 7 内存](../../chapter-07-memory/)。

---

### 三、Linux 现代性能焦点（三件）

#### 1. systemd

- 现代服务管理器；**`systemd-analyze`** 可分解**启动时间**（哪 unit 慢）。
- HFT 裸机：关注 **服务依赖、After=、是否拖慢共置机器就绪**；热路径进程常不由 systemd 频繁重启。
- 性能视角的两个坑：① unit 依赖链串行化拖慢开机就绪（交易日的开盘前检查窗口有限）；② 默认 cgroup 分组让所有服务都进 cpu.max 限额的管辖范围（见 [11.3](../../chapter-11-cloud-computing/notes/section-11.3-操作系统虚拟化-容器.md)）——热路径服务要么显式 unlimited，要么干脆不归 systemd 管。

#### 2. KPTI（Meltdown 缓解）

- **内核页表隔离**：修复 CPU 侧信道漏洞，增加 **syscall / 上下文切换** 时的页表切换与 **TLB 刷新** 开销。
- 影响：**约 0.1%–6%**（workload 依赖）；syscall 密集或切换频繁时更明显。
- 机制直觉：KPTI 之前用户页表和内核页表是同一张（内核部分对用户态不可见但无需切换）；KPTI 之后每次进内核要**切到完整页表、出内核再切回来**，两次 CR3 写 + TLB 冲刷。syscall 密集型负载（每次进出都付税）与「一次进来干很久」的负载（摊薄了税）差异巨大。
- HFT：评估是否可用 **PCID**（给页表打标签避免全量冲刷）、内核版本、mitigations 开关（`mitigations=off`，与安全合规权衡）→ 与 [14-HFT ch05](../../../14-hft-engineering/chapter-05-os-kernel-tuning/README.md) 对照。

#### 3. Extended BPF（eBPF）

- 内核态 **安全虚拟机**：验证器先行检查（确保不可死循环、不可越界访问），通过后在内核里运行——**可编程观测**（tracepoint、kprobe、uprobe、XDP/tc-BPF…）。
- 驱动 **BCC、bpftrace** 等 — Gregg 称「当前最重要技术之一」。
- 对 HFT 的意义分两面：观测面（全书主线——低开销事件级诊断的唯一通用方案）；数据面（XDP/tc-BPF 可做超低延迟包处理，是 DPDK 之外的内核内旁路选项）。

```
Ch 3 知道「BPF 能在内核里安全插桩」
  → Ch 4 选工具
  → Ch 15 + 06.7 系统学
```

---

### HFT / 嵌入式关联

| 里程碑 | HFT 动作 | 嵌入式动作 |
|--------|----------|-----------|
| CFS | 热路径不用公平份额——绑核 + SCHED_FIFO（[6.9 调优](../../chapter-06-cpus/notes/section-6.9-CPU-调优.md)） | RT 任务用 SCHED_FIFO/RR，注意 sched_rt_runtime_us 兜底 |
| RCU | 内核读路径免锁——理解 /proc 读取为何可以便宜 | 同左；实时内核上 RCU 回调走 rcuc 线程（rcu_nocbs 相关） |
| epoll | 行情/订单网关的标准并发模型 | 中低端嵌入式常用 epoll 替代多线程轮询 |
| cgroups | 共置机器隔离邻居；热路径服务绕开限额 | 多为单用途设备，用不上 |
| THP | 关 THP + hugetlbfs 显式预留 | 小内存设备通常直接关 |
| KPTI | 评估 PCID/mitigations 开关与合规的权衡 | 新 ARM SoC 无 Meltdown 问题（架构不同），x86 嵌入式同左 |
| eBPF | 观测主力 + XDP 数据面选项 | 目标机常无 BTF/headers，走 libbpf+CO-RE 预编译（[4.1](../../chapter-04-observability-tools/notes/section-4.1-工具覆盖范围与危机工具.md)） |

---

### 衔接

- 上一节 [3.2 内核基础](./section-3.2-内核基础与核心概念.md)：里程碑背后机制的正文。
- 下一节 [3.5 其他系统模型](./section-3.5-其他系统模型.md)：不走宏内核路线的另几种选择。
- 设计脉络：[LKD Ch 1 简介](../../../05-linux-kernel/chapter-01-intro/)（Unix 基因与 Linux 对比）。
- KPTI 的上下文切换成本细节 → [11.2 虚拟化](../../chapter-11-cloud-computing/notes/section-11.2-硬件虚拟化Hardware-Virtualization.md) 的 EPT/VM-EXIT 同族问题。

---

### 常见陷阱

1. 把 VFS/slab/DTrace 思想当成 Linux 原创——血统来自 Solaris/BSD；跨 Unix 的方法论可以迁移
2. THP 无脑开——TLB 收益真实，但运行时 compaction 尖刺是延迟杀手；HFT 用显式大页预留替代
3. KPTI 影响按「固定百分比」理解——syscall 密集型在谱高端（数个 %），长驻计算型在低端（<1%）
4. systemd 只当「开机服务」看——它的 cgroup 分组决定了每个服务的 CPU 限额管辖范围
5. 里程碑当历史知识背——六个里程碑各对应一层性能分析地图（调度/同步/IO复用/隔离/TLB/虚拟化），出问题先想自己在哪一层

<details>
<summary>自测题（点击展开）</summary>

1. epoll 相对 select/poll 的本质优势是什么？
   <details><summary>答</summary>内核维护就绪队列，epoll_wait 只返回就绪 fd（O(1)）；select/poll 每次全量拷贝+线性扫描（O(n)，与 idle 连接数成正比）——万级连接下差距随连接数线性拉大</details>
2. THP 的收益和代价各是什么？HFT 的标准处置？
   <details><summary>答</summary>收益：2MB 页让 TLB 覆盖 ×512，热路径 TLB miss 大降；代价：运行时分配可能触发 compaction（ms 级停顿）。HFT 标准：关 THP、用 hugetlbfs 显式预留——留收益去惊喜</details>
3. KPTI 为什么对 syscall 密集型负载伤害更大？
   <details><summary>答</summary>KPTI 每次进出内核都要切页表（CR3 写+TLB 冲刷）——syscall 密集 = 每次付税；长驻计算型一次进来干很久，税被摊薄。缓解：PCID 打标签避免全量冲刷，或 mitigations=off（安全权衡）</details>
4. eBPF「安全虚拟机」的「安全」由什么保证？
   <details><summary>答</summary>验证器在加载时静态检查：不可死循环（有界跳转）、不可越界访问内存、只允许调用白名单函数——通过后才能进内核运行，这是它能被生产环境接受的前提</details>
5. Unix 血统认知对性能工程师有什么实际价值？
   <details><summary>答</summary>概念与方法论可跨血统迁移——DTrace 的探针/聚合思维直接适用于 bpftrace；Solaris 的 kstat 方法论在 /proc+eBPF 有对应物。工具会换，方法论连续</details>

</details>


---

← [本章导读](../README.md)
