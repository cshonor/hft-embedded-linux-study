# 第5章 操作系统内核极致调优

> **原书第 6 章 §1 · 最小化上下文切换** + Linux 落地  
> **CPU 隔离 · Kernel Bypass · Huge Pages**

← 原理：[chapter-04 硬件到 OS](./chapter-04-硬件选型与服务器配置.md) · 无锁/内存池：[chapter-07](./chapter-07-无锁数据结构与内存布局.md)

---

## 本章定位

原书 **Ch6 HFT Optimization（架构与 OS）** 的第一支柱：**消灭上下文切换** — 本章给出 **代价模型 + Linux 实操**；第二、三支柱（无锁、内存池）→ [chapter-07](./chapter-07-无锁数据结构与内存布局.md)。

---

## 1. 上下文切换的代价

**Context Switch：** OS **保存** 当前线程/进程状态，**恢复** 另一线程运行。

| 开销 | HFT 视角 |
|------|----------|
| **保存 PCB** | 寄存器、栈指针等 — CPU 周期 |
| **Cache Invalidation** | 新线程 **冷缓存** |
| **TLB 刷新** | 页表映射缓存失效 → **内存 walk** |
| **恢复后 Cache Miss** | 从 **RAM（~60ns 级）** 重拉代码/数据 — 原 [chapter-04 §1](./chapter-04-硬件选型与服务器配置.md) L1 **~0.5ns** 对比 |

**对 HFT：** 切换不仅是 **μs 级调度** — 更致命的是 **破坏 L1/L2 工作集**，原线程 **恢复延迟暴增**。

**触发源：** 公平调度 · **I/O 阻塞 syscall** · **锁等待** · **IRQ** — 热点路径应 **逐一剔除**。

---

## 2. 优化：CPU 隔离与核心绑定

**违背** OS 默认公平调度与节能 — **为延迟牺牲吞吐**。

| 手段 | 说明 |
|------|------|
| **`taskset` / `pthread_setaffinity`** | 热点线程绑 **独占物理核** |
| **`isolcpus=`** | 核心 **不参与 CFS 通用队列** |
| **`nohz_full`** | 减少 **定时器 tick** 打断 |
| **IRQ affinity** | 网卡中断 **远离** 热点核 · 或 **用户态 poll** 无 IRQ |

**拓扑：**

```
Core 0: Gateway IN (poll)
Core 1: Book Builder
Core 2: Strategy
Core 3: Gateway OUT
(其余核: 日志/监控 — 可共享)
```

**禁止：** 热点线程与 **HT 兄弟核** 同跑重负载。

→ [06-The-Linux-Programming-Interface 调度](../06-The-Linux-Programming-Interface/)

---

## 3. 减少阻塞：Kernel Bypass

**阻塞式 `recv()`** → 线程睡眠 → **必然切换**。

| 对策 | 效果 |
|------|------|
| **User-space spinning** | **100% poll** NIC ring — 无 IRQ/阻塞 |
| **Zero copy** | DMA → 用户映射 · 无 memcpy |
| **OpenOnload / VMA / DPDK** | 商用 Bypass |
| **Busy-poll socket**（次选） | 仍经内核 |

| 效果（原书 Ch7） | **1.5–10 μs → 0.5–2 μs** |

→ [chapter-06 §5](./chapter-06-低延迟网络与协议优化.md#5-数据包生命周期kernel-路径) · [chapter-10 §1](./chapter-10-延迟测量与基准压测.md#1-内核旁路-kernel-bypass) · [12-DPDK](../12-DPDK-Low-Latency-Network/)

---

## 4. BIOS / 电源：降低 Jitter

| 禁用 | 原因 |
|------|------|
| **Hyper-Threading** | 争用执行单元 · 不可控交替 |
| **C-states / P-states** | 唤醒惩罚 |
| **Turbo Boost** | 频率跳变 |

---

## 5. Huge Pages（与 TLB）

**切换加剧 TLB 压力** — 大页 **减少 miss 概率**（原理 [chapter-04 §2](./chapter-04-硬件选型与服务器配置.md)）。

```bash
echo 1024 > /proc/sys/vm/nr_hugepages
# 应用 mmap(MAP_HUGETLB) 或 libhugetlbfs
```

**与 Memory Pool 分工：** 大页 **映映射**；对象 **池化** → [chapter-07 §3](./chapter-07-无锁数据结构与内存布局.md#3-内存预分配与缓存友好)

---

## 本章小结

| 目标 | 手段 |
|------|------|
| **少切换** | Pinning · isolcpus · 无阻塞 I/O |
| **少 Jitter** | 关 HT/Turbo/C-states |
| **少 syscall 路径** | Kernel Bypass |
| **少 TLB miss** | Huge Pages |

**下一支柱：** [chapter-07 无锁 + 内存池](./chapter-07-无锁数据结构与内存布局.md)
