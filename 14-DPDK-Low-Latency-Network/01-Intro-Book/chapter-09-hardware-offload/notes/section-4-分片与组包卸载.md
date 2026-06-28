## 4. 分片与组包卸载

---

### 一、TSO — TCP Segmentation Offload（发送）

**场景：** 应用要发 **大于 MTU** 的 TCP 载荷。

| 软件 | 硬件 TSO |
|------|----------|
| 切分为多个 **≤MTU** 段，每段 **复制 TCP 头**、改长度、算 checksum | NIC **一次收大块**，自动 **切分 + 更新头 + checksum** |

- **方向：** 仅 **TX**  
- **收益：** 极大减 **CPU 与 PCIe 事务数**（大流发送）  
- **HFT：** **TCP 发单** 路径可考虑；**UDP tick** 通常 **无 TSO**

---

### 二、RSC — Receive Side Coalescing（接收）

**TSO 的逆过程 — RX 方向：**

```
多个 TCP 小分片 → 硬件缓存、排序 → 聚合成 **一个大分片** → 交给软件
```

| 收益 | 说明 |
|------|------|
| 少 **包头解析** 次数 | 软件一次处理更大 PDU |
| 少 **排序/重组** 负担 | 硬件已 coalesce |

**注意：** 聚合改变 **逐包粒度** — 低延迟场景需评估 **是否增加 batch 延迟**。

---

### 三、与 MTU / 线速

- TSO/RSC 与 [Ch7 队列深度](../chapter-07-nic-performance-optimization/notes/section-5-队列长度及阈值设置.md)、[Ch6 PCIe 带宽](../chapter-06-pcie-packet-io/notes/section-5-PCIe净荷带宽计算.md) 联动  
- 开启前确认 **PMD + 网卡能力**（`dev_info` / `rx/tx_offload_capa`）

---

← [3. 计算及更新卸载](./section-3-计算及更新功能卸载.md) · 下一节 [5. DPDK 协同](./section-5-DPDK软件接口与协同.md)
