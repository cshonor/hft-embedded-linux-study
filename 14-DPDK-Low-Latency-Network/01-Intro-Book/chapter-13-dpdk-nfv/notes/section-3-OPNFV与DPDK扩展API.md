## 3. OPNFV 开源生态与 DPDK 扩展 API

---

### 一、OPNFV（2014–）

**目标：** 整合开源栈推动 **NFV 落地**

| 常见组件 | 与 DPDK |
|----------|---------|
| **OpenStack** | 编排、租户 VM |
| **KVM** | 虚拟化 |
| **OVS / OVS-DPDK** | 虚拟交换 — **依赖 DPDK 加速** |
| **DPDK** | **数据面统一加速引擎** |

大量 NFV 项目 **直接/间接** 依赖 DPDK — 与 [Ch12 vhost-user](../chapter-12-vhost-optimization/) 同生态。

---

### 二、领域扩展 API（NFV 向）

在 **PMD/mbuf/ring** 之上，DPDK 提供 **统一硬件资源入口**：

| API | 用途 | 加速手段 |
|-----|------|----------|
| **CryptoDev** | IPSec、SSL、报文加解密 | **AES-NI**、**Intel QAT** 等 |
| **Pattern Matching** | 正则/规则匹配 | **Hyperscan** 软件库 |
| **Compression** | 传送/存储压缩解压 | 硬件或优化算法 |

**意义：** VNF（防火墙、DPI、VPN）不必各自造轮子 — **走 DPDK 抽象**。

→ [Ch3 SIMD](../chapter-03-parallel-computing/notes/section-4-数据并行与SIMD.md) · [Ch9 硬件 offload](../chapter-09-hardware-offload/notes/section-3-计算及更新功能卸载.md)

---

### 三、与全书技术栈映射

```
Ch4  ring / 同步  →  VNF 核间传递
Ch5  Hash/LPM/ACL  →  vRouter / vFW 查表
Ch9  Checksum/offload →  与 CryptoDev 互补
Ch10–12 虚拟化     →  VNF 运行环境
Ch13  CryptoDev…   →  安全类 VNF 算力
```

---

← [2. NFV 架构](./section-2-NFV起源与架构.md) · 下一节 [4. VNF 评估](./section-4-VNF评估与性能分析.md)
