## 2. PCIe 事务与带宽

---

### 一、PCIe 协议栈

PCIe 分层（自顶向下）：

```
事务层 (Transaction Layer)  ← TLP
数据链路层 (Data Link Layer)
物理层 (Physical Layer)
```

网卡 **DMA 控制器** 访问主机内存，本质是 CPU/Root Complex 与设备间交换 **事务层包 (TLP)**。

| TLP 类型 | 含义 |
|----------|------|
| **MRd** | Memory Read — 设备读主机内存 |
| **MWr** | Memory Write — 设备写主机内存 |
| 其他 | Config、Completion 等 |

---

### 二、TLP 开销

完整 TLP 除 **净荷 (Payload)** 外还含：

- 头部 (Header)
- 序列号、校验 (LCRC/ECRC 等，视代际与实现)

→ 承载应用数据时，**每事务额外几十字节** 协议开销，有效带宽 **低于** 物理线速。

---

### 三、代际与理论带宽

| 代际 | 编码 | 每 Lane 每方向（约） |
|------|------|---------------------|
| **Gen1** | 8b/10b | ~250 MB/s |
| **Gen2** | 8b/10b | ~500 MB/s |
| **Gen3** | 128b/130b | ~985 MB/s |
| **Gen4+** | 128b/130b | 继续倍增 |

**有效带宽** 还受硬件实现约束：

- TLP 需从特定 **时钟周期 / Lane** 边界发起  
- MPS (Max Payload Size)、MRRS 等配置  
- 读写混合、Completion 拆分  

→ 实测常 **显著低于**  datasheet 峰值 — 小包场景更突出（→ §5）。

---

### 四、与包处理的关系

每个包的 **描述符读写 + 帧数据 DMA** 均产生多条 TLP — 评估 **PPS 天花板** 必须算 **PCIe 总事务量**，不能只看 64B 帧长。

→ [Ch2 DDIO/NUMA](../chapter-02-Cache与内存/notes/section-6-DDIO与NUMA.md) · [02-Hennessy I/O](../../../02-Computer-Architecture-6th/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. DMA 描述符环](./section-3-DMA描述符环形队列.md)
