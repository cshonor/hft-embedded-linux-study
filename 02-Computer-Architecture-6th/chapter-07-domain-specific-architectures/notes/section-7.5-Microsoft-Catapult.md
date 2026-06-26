## 7.5 Microsoft Catapult (FPGA)

### 定位

| 属性 | 说明 |
|------|------|
| **形态** | 数据中心 **FPGA** 加速器 |
| **部署** | **Bump-on-a-wire** — 串接在网络路径上（NIC 旁） |
| **初期用途** | Bing 搜索排序：特征提取、ML 打分 |
| **扩展** | CNN 等可重配置 workload |

---

### 编程模型

| 层次 | 说明 |
|------|------|
| **Shell** | 系统级外壳 — PCIe、DMA、网络接口 |
| **Role** | 应用级逻辑 — 开发者专注算法 |

降低 RTL 全栈难度，支持 **部分重配置**。

| HFT 视角 |
|----------|
| **最接近交易场景的 DSA 案例之一**：在 **网线路上** 做解析/过滤/聚合 |
| SmartNIC、FPGA 行情解码、**inline checksum** — 同「bump-in-the-wire」哲学 |
| 灵活度 **FPGA > ASIC**；峰值能效 **ASIC > FPGA** — 量产后才固化 ASIC |
| → [14-DPDK](../../../14-DPDK-Low-Latency-Network/) · [15-HFT 硬件选型](../../../15-HFT-Low-Latency-Practice/chapter-04-硬件选型与服务器配置.md) |

---
