## 3. 硬件流分类 (Flow Classification)

> 网卡 **硬件** 识别流 → 写描述符 / **导向队列** — 卸载 CPU 解析

---

### 一、流分类是什么

网卡依据包 **特性** 分类，将结果：

- 记录在 **RX 描述符** 中，或  
- **直接** 把流导入 **特定队列**

DPDK 通过 **mbuf** 读标志 — **免软件 parse 包头**。

---

### 二、包类型识别（Packet Type Offload）

高级网卡（如 **Intel XL710**）在硬件识别：

- **L3/L4**：IPv4/IPv6、TCP、UDP  
- **隧道**：VXLAN、NVGRE 等  

信息挂在 **接收描述符 / mbuf ol_flags** — 应用 **跳过** 逐层解封装判断。

---

### 三、RSS（Receive Side Scaling）— 负载均衡

| 步骤 | 说明 |
|------|------|
| **关键字** | 常取 **四元组**（IP + 端口）等 |
| **哈希** | 如 **Toeplitz** 算法 → hash 值 |
| **散列** | hash **模队列数** → 分配到 **RX queue** |

**对称哈希 (Symmetric Hash)：**  
正向与反向流（A→B 与 B→A）→ **同一队列 / 同一核** — **防火墙、会话表** 必需。

**HFT：** 无状态 **tick 解码** 常用 RSS **Spread** 多核；有 **per-flow 状态** 时考虑对称哈希或 **FD**。

---

### 四、Flow Director — 精确匹配

Intel **精确匹配** 技术：

- 网卡内 **匹配表** — 按 **特定 TCP 会话** 等字段  
- 命中 → **导向预留的特定队列**

**用途：** 少量 **控制流 / 会话** 从海量转发流量中 **硬件剥离** — CPU **零过滤**。

---

### 五、QoS（服务质量）

依据 **VLAN UP（用户优先级）** 等：

- 划分 **Traffic Class (TC)**  
- TC 间 **Weighted Strict Priority** 等 **硬件调度** — 保障高优先级带宽  

HFT：共置机 **裸金属** 较少用 VLAN QoS；**托管/NFV** 场景更常见。

---

← [2. 多队列](./section-2-网卡多队列.md) · 下一节 [4. 实战](./section-4-DPDK实战结合.md)
