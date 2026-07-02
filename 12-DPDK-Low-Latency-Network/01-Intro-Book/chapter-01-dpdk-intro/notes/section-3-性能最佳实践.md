## 3. DPDK 突破性能瓶颈的最佳实践

> 早年 IA 多核被认为 **不适合** 高速包处理 — DPDK 用工程实践 **证伪**

---

### 一、轮询模式 (Poll Mode)

| 传统 | DPDK |
|------|------|
| **网卡中断** → 上下文切换、softirq | **轮询** 收包 — **无中断开销** |

→ 深潜：[chapter-03 PMD与轮询模式](../chapter-03-PMD与轮询模式.md) · [ULK Ch4 I/O 中断](../../../04-Understanding-Linux-Kernel/chapter-04-interrupts-and-exceptions/notes/section-6-IO中断处理.md)

**代价：** 占满 CPU 核 — 需 **绑核**、isolcpus，与 idle 友好性 trade-off。

---

### 二、用户态驱动 (User-space Driver)

- 网卡驱动运行在 **用户态**  
- **减少** 内核 ↔ 用户 **内存拷贝**  
- **避免** 频繁 **系统调用** 延迟  

即 **PMD（Poll Mode Driver）** 体系 — 数据面完全在用户态闭环。

---

### 三、亲和性与独占

| 做法 | 收益 |
|------|------|
| **CPU 亲和性绑定** | 线程固定逻辑核 |
| **独占 lcore** | 避免跨核迁移 → **Cache miss** ↓ |

→ [ULK Ch7 调度与 affinity](../../../04-Understanding-Linux-Kernel/chapter-07-process-scheduling/notes/section-6-调度相关系统调用.md) · [15 HFT 绑核](../../../15-HFT-Low-Latency-Practice/)

---

### 四、降低访存开销

| 技术 | 作用 |
|------|------|
| **Hugepages（大页）** | ↓ TLB miss |
| **NUMA 感知** | 内存/网卡 **同节点** 分配 |
| **Intel DDIO** 等 | 网卡 DMA 数据 **直达 Cache** — ↓ 内存带宽压力 |

→ [ULK Ch8 ZONE/伙伴](../../../04-Understanding-Linux-Kernel/chapter-08-memory-management/) · [CSAPP Ch6 缓存](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/)

---

### 五、IA 新硬件：SIMD 与超标量

- **SIMD** — 单指令多数据，批量处理包头/字段  
- **超标量** — 指令级并行  

在 **数据面** 做深度向量化 — 与 [Hennessy SIMD/GPU](../../../02-Computer-Architecture-6th/chapter-04-vector-simd-gpu/) 概念呼应。

---

← [2. 硬件平台](./section-2-硬件平台与DPDK定位.md) · 下一节 [4. 方法论](./section-4-底层方法论.md)
