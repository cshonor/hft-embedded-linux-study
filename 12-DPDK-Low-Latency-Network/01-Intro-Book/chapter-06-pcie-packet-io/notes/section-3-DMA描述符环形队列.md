## 3. DMA 描述符环形队列

---

### 一、队列结构

DMA 控制器通过 **环形队列** 与 CPU 协作：

| 组件 | 作用 |
|------|------|
| **描述符数组** | 物理 **连续** 内存，每项描述一块缓冲区 |
| **控制寄存器** | **Base**（基址）、**Size**（深度）、**Head**、**Tail** |

```
        Base ──→ [ desc0 | desc1 | ... | descN-1 ]
                    ↑ Head          ↑ Tail
```

- **硬件** 通常维护 Head（已处理位置）  
- **软件** 通过移动 **Tail** 通知硬件「有新描述符可用」

---

### 二、接收 (RX)

1. 软件将 **空闲缓冲区物理地址** 填入描述符  
2. 更新 **Tail** — 告知 NIC 可 DMA 写入  
3. 包到达 → 硬件 **DMA 写数据** 到该缓冲区  
4. 硬件置描述符 **完成位 (DD, Descriptor Done)**  
5. 软件轮询/检查 DD → 取走 mbuf → **重填描述符**

---

### 三、发送 (TX)

1. 软件将 **待发送数据地址/长度** 填入描述符  
2. 更新 **Tail** — 触发硬件发送  
3. 发送完成 → 硬件更新 **DD**  
4. 软件回收描述符 / 释放 mbuf

---

### 四、与 DPDK PMD 的关系

- PMD **poll mode** 批量检查 DD、批量 refill — 减少 per-packet 寄存器访问（→ §4）  
- 描述符环深度、Prefetch 与 [Ch2 Cache 预取](../chapter-02-cache-and-memory/notes/section-3-Cache预取.md) 影响 miss 率  

→ 内核 NAPI 环对照 [13-LKN](../../../11-Linux-Kernel-Networking/) · PMD 深潜 [chapter-03-PMD](../chapter-03-PMD与轮询模式.md)

---

← [2. PCIe 事务与带宽](./section-2-PCIe事务与带宽.md) · 下一节 [4. CPU 与 I/O 优化](./section-4-CPU与IO协奏优化.md)
