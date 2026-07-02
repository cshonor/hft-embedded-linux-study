## 3. 计算及更新功能卸载

---

### 一、VLAN 硬件卸载

| 软件做法 | 硬件 offload |
|----------|--------------|
| 拷贝 + 手工剥/插 **802.1Q Tag** | RX：**过滤 + 剥离**；TX：**自动插入** |

- 依赖网卡 **VLAN 过滤表** 配置  
- 减 **内存拷贝** 与分支 — [Ch6 mbuf head room](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md) 仍可配合封装  

---

### 二、IEEE 1588 (PTP) 硬件卸载

**用途：** 网络设备 **精准时间同步**（共址、事件排序）。

| | 软件打戳 | 硬件打戳 |
|---|---------|---------|
| 位置 | 协议栈深处 | **近物理层** |
| 精度 | 延迟 **抖动大** | **纳秒级** 稳定 |

**HFT：** 行情 **到达时间戳**、跨机对时 — PTP 硬件 offload 与 [15 HFT 时钟](../../../15-HFT-Low-Latency-Practice/) 同主题。

---

### 三、Checksum 硬件卸载

IP / TCP / UDP / SCTP 校验和：**逻辑简单、但要扫完整包** — CPU 开销大。

| 方向 | 硬件行为 |
|------|----------|
| **RX** | 自动校验，错误包可标记丢弃 |
| **TX** | 自动 **计算并写入** 校验和字段 |

→ DPDK 通过 `ol_flags` 声明（§5）· 与 [Ch3 CRC SIMD](../chapter-03-parallel-computing/notes/section-4-数据并行与SIMD.md) 软件路径对照

---

### 四、Tunnel 硬件卸载

Overlay：**VxLAN、NVGRE** 等。

- 基于 **内层 IP/MAC** 或 **VNI/TNI** 做 **重定向、过滤**  
- 减 CPU 解析外层隧道头 — 与 **SDN / 数据中心** 场景更相关；HFT 共置 UDP 组播 **较少用**，但 **智能网卡** 能力谱系一环  

→ [Ch8 ptype / 包类型](../chapter-08-flow-classification-multiqueue/notes/section-3-硬件流分类.md)

---

← [2. 硬件卸载简介](./section-2-硬件卸载简介与演进.md) · 下一节 [4. TSO / RSC](./section-4-分片与组包卸载.md)
