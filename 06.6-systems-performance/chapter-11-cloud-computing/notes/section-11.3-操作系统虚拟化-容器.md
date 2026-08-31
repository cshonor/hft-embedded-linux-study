## 11.3 操作系统虚拟化 / 容器

> 章节导航：[11.2 硬件虚拟化](./section-11.2-硬件虚拟化Hardware-Virtualization.md) · 上一篇 ← · 下一篇 [11.4 MicroVM](./section-11.4-轻量级硬件虚拟化MicroVM.md) · [本章导读](../README.md)

**本节讲什么**：容器 = namespaces + cgroups 组合的机制本质、cgroups v2 的资源控制与 throttle 证据链、容器观测的系统性陷阱（工具显示的是宿主机视图）、以及「裸机 + cgroup 隔离」的 HFT 用法。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 容器**不是内核对象** | 是 ns + cgroup 的组合视图 |
| 2 | CPU 路径**几乎零开销** | 同一内核，无 VM-EXIT |
| 3 | 但**物理 cache/TLB/带宽不隔离** | 容器 ≠ 魔法隔离 |
| 4 | `cpu.stat` 的 `nr_throttled` 是 **throttle 铁证** | 只看 CPU% 会漏 |
| 5 | 容器内传统工具显示的是**宿主机视图** | 观测要按 cgroup 口径 |

---

### 一、机制本质：Namespaces + cgroups

| 机制 | 隔离什么 | 性质 |
|------|----------|------|
| **Namespaces** | PID、NET、MNT、UTS、IPC、USER（+cgroup ns） | **视图隔离**——进程看到的世界 |
| **cgroups** | CPU、内存、blkio/io、pids | **资源限额与统计**——进程能用多少 |

容器在内核里**没有单一对象**——Docker/LXC/containerd 做的事情是：建一组 namespace（新视图）+ 挂进 cgroup（限额）+ 叠加文件系统（overlayfs）。「容器」是用户态编排出来的概念。

这就是理解容器一切性能行为的钥匙：
- **快**：没有 hypervisor 层，syscall 直接进内核（对比 [11.2 的 VM-EXIT 税](./section-11.2-硬件虚拟化Hardware-Virtualization.md)）。
- **不彻底**：视图隔离不等于资源隔离——内核锁、物理 cache、内存带宽、TLB 全共享。

### 二、CPU 路径几乎零开销

| 方面 | 容器 | VM |
|------|------|-----|
| syscall 路径 | 直接进内核，**无额外层** | 敏感操作 VM-EXIT |
| 内存访问 | 原生页表 | EPT 嵌套翻译 |
| 启动速度 | 毫秒级 | 秒级 |
| 隔离强度 | 共享内核（逃逸面大） | 独立内核 |
| **主要问题** | **共享物理 cache/TLB/内核锁** | hypervisor 税 |

**HFT 启示**：容器**不会** magically 隔离 L3 cache——邻居容器冲刷 LLC 时你的热点数据照样被逐出（伪共享的跨容器版，[ch13 c2c](../../chapter-13-perf/notes/section-13.12-其他常用能力延伸.md) 同类问题）。延迟敏感场景的隔离靠**物理隔离或 dedicated 核**，容器只是打包便利。

### 三、cgroups 资源控制（v2）

**CPU：**

| 控制 | 文件/概念 | 效果 |
|------|-----------|------|
| **weight (shares)** | `cpu.weight` | 相对权重，争抢时生效 |
| **bandwidth（硬 cap）** | `cpu.max` | 如 `max 200000 100000` = 每 100ms 周期最多 200ms CPU 时间（2 核） |
| **throttle 证据** | `cpu.stat` → `nr_throttled` / `throttled_usec` | 触顶的次数与累计时长 |

**⭐ throttle 机制**（为什么 limit 造成延迟尖刺）：`cpu.max` 按**周期配额**控制——周期开始发满配额，用完后**整个 cgroup 被冻结到下个周期**。4 线程进程配 2 核 quota 时，4 线程并行 50ms 就耗光 100ms 配额，随后**全冻结 50ms**——表现为规律性的延迟尖刺，而不是平滑减速：

```
CPU 时间
  ▲ 2 核满速 ──┐
  │            └──冻结──┐      ┌──冻结──┐
  │                    └──────┘        └─── ...   ← 每 100ms 一个 cliff
  └─────────────────────────────────▶ 时间
```

**内存 / IO：**

- `memory.max`（硬顶，触发 OOM kill）、`memory.high`（软顶，触发 throttle/reclaim）
- `memory.events`（OOM 计数）——容器重启的真实原因常在这里
- `io.max`（v2）——IOPS/带宽限额

```bash
# 容器/服务是否被 CPU 节流（cgroup v2）
cat /sys/fs/cgroup/cpu.stat
# nr_periods nr_throttled throttled_usec ...
# nr_throttled > 0 且在涨 = 延迟尖刺的 cgroup 级证据
```

### 四、观测陷阱（Gregg 重点）

**容器内运行 `top` / `iostat` / `uptime` / `mpstat`——显示的是宿主机视图**：

| 工具显示 | 实际可能是 |
|----------|------------|
| 8 CPU 全 busy | **宿主机 8 核**；容器可能只 **quota 2 核**，另 6 核是别人的 |
| Load average 很高 | **宿主机 load**，非本容器 cgroup 的运行队列 |
| `%iowait` | 宿主机全局值 |
| free 内存 | 宿主机的（cgroup 限额要看 `memory.current`） |

**正确观测矩阵：**

| 层级 | 看什么 |
|------|--------|
| **容器内** | cgroup 文件：`cpu.stat`、`memory.current`、`memory.events`；`/sys/fs/cgroup/` 就是容器自己的真相 |
| **宿主机** | `systemd-cgtop`、BPF 按 cgroup 过滤、`kubectl top`（API 聚合层） |
| **K8s** | cAdvisor/Prometheus 的 **CPU throttling 指标**（本质就是 nr_throttled 的时序化） |

```bash
# 容器内自己是不是瓶颈：quota、用量、throttle 三件套
cat /sys/fs/cgroup/cpu.max
cat /sys/fs/cgroup/cpu.stat
# 宿主机上按 cgroup 追踪（bpftrace，需宿主权限）
# bpftrace -e 'tracepoint:sched:sched_switch { @[cgroup] = count(); }'
```

**判断口诀**：容器内工具看「世界的样子」，cgroup 文件看「自己的份额」——延迟归因永远用后者。

### 五、裸机 + cgroup 的 HFT 用法

HFT 裸机上不用 Docker 也该用 cgroup/systemd 划资源边界：

```
/etc/systemd/system/strategy.service
  [Service]
  CPUAffinity=2 3          # 绑核
  CPUQuota=200%            # 等价 cpu.max 硬顶
  MemoryMax=32G
  AllowedCPUs=2-3
```

即使 dedicated 核，也用 cgroup 限额防**失控进程**（内存泄漏吞掉整机）——并周期巡检 `cpu.stat`/`memory.events`，勿只信 `top`。这与 [ch6 CPU 调优](../../chapter-06-cpus/)的隔离栈（isolcpus + affinity + cgroup）是同一层的互补工具。

### 衔接

- 上一节：[11.2 硬件虚拟化](./section-11.2-硬件虚拟化Hardware-Virtualization.md)
- 下一节：[11.4 MicroVM](./section-11.4-轻量级硬件虚拟化MicroVM.md)（容器速度 + VM 隔离的折中）
- 关联：[ch4 观测工具](../../chapter-04-observability-tools/)、[ch15 BPF](../../chapter-15-bpf/)（按 cgroup 过滤）、[ch6 cgroups](../../chapter-06-cpus/)

---

### 常见陷阱

1. **容器内 top 判断瓶颈**——显示宿主机 8 核而自己 quota 2 核，误判「CPU 还有富余」。
2. **CPU limit 造成规律性尖刺不知道**——quota 冻结机制是 cliff 不是斜坡，`nr_throttled` 是铁证。
3. **以为容器隔离了 cache**——namespaces 不隔离 LLC/内存带宽/内核锁，物理隔离才是隔离。
4. **容器重启原因不查 memory.events**——OOM kill 计数在 cgroup 文件里，不在应用日志里。

<details>
<summary>自测题（点击展开）</summary>

1. 容器在内核里是什么？
   <details><summary>答</summary>没有单一对象——是 namespaces（视图隔离）+ cgroups（资源限额）+ overlayfs 的用户态组合概念。</details>
2. cpu.max 的 throttle 为什么造成延迟尖刺而非平滑减速？
   <details><summary>答</summary>按周期发配额：用光后整个 cgroup 冻结到下个周期——并行线程越多消耗越快，冻结窗口越明显，表现为周期性 cliff。</details>
3. 容器内 top 显示 8 核忙，怎么确认自己的真实额度？
   <details><summary>答</summary>读 /sys/fs/cgroup/cpu.max（quota）和 cpu.stat（用量+throttle）——容器内传统工具显示宿主机视图。</details>
4. 容器和 VM 的隔离本质差异？
   <details><summary>答</summary>容器共享内核（syscall 零开销但逃逸面大、物理资源不隔离）；VM 独立内核（hypervisor 税但边界硬）。</details>
5. 裸机 HFT 为什么也用 cgroup？
   <details><summary>答</summary>给失控进程设硬边界（内存泄漏不吞整机）+ systemd 绑核限额一体管理——不是隔离邻居，是防御自己。</details>

</details>


---

← [本章导读](../README.md)
