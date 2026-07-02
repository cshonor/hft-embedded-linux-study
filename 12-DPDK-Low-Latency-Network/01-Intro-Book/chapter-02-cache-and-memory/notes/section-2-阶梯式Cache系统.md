## 2. 阶梯式 Cache 系统

> 弥补 **CPU 与内存** 之间的速度鸿沟

---

### 一、L1 / L2 / L3

| 级别 | 特点 |
|------|------|
| **L1** | **最快、最小** — 分 **指令 Cache (I)** 与 **数据 Cache (D)**；**每核独占** |
| **L2** | 稍慢、容量更大 — 通常 **每核独占** |
| **L3 (LLC)** | **最大** — **所有核心共享** Last Level Cache |

包处理热路径：**命中 L1/L2** = 纳秒级；**落内存** = 数百 cycle。

→ [01-CSAPP Ch6](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) · [02-Hennessy Ch2](../../../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/)

---

### 二、Cache Line（缓存行）

- CPU 与内存交换的 **基本单位** — 通常 **64 字节**  
- 读 1 字节也可能 **整行载入** — 结构体布局影响 **false sharing**（→ [section-4](./section-4-Cache一致性与无锁设计.md)）

**HFT：** 热数据结构 **≤ 64B 或按行对齐拆分**；避免无关字段与高频计数器 **同行**。

---

### 三、TLB Cache

| 作用 | 缓存 **虚拟地址 → 物理地址** 的页表项 |
|------|----------------------------------------|
| **TLB miss** | CPU 需 **遍历多级页表** 访存 — 极贵 |

→ [ULK Ch2 页表](../../../04-Understanding-Linux-Kernel/chapter-02-memory-addressing/) · 大页缓解：[section-5](./section-5-大页Hugepages.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 预取](./section-3-Cache预取.md)
