## 2. 硬件卸载简介与演进

---

### 一、协同化软硬件设计

**软件优化 + 硬件加速** = 系统优化基石。

| 平面 | 职责 | 例子 |
|------|------|------|
| **控制面** | 复杂策略、表项配置、慢路径 — **软件** | 路由协议、ACL 规则下发 |
| **数据面** | 查表、校验、改头、加解密等 **简单重复** — 理想由 **硬件** | checksum、VLAN tag、TSO |

目标：**提高吞吐、降低时延**，释放 CPU 做更高层逻辑。

**HFT 视角：** 行情网关热路径中，checksum 验证、VLAN 剥离等 **机械运算** 交给网卡 — CPU 只做 **行情解析和策略决策**。

---

### 二、专有设备 → 现代网卡

传统 **NP/ASIC** 数据面高度硬件化；通用服务器 **Intel i350 / 82599 / XL710 / E810** 等也逐步提供丰富 offload。

**三大类（本书划分）：**

| 类 | 代表 | 方向 | DPDK 配置 |
|----|------|------|-----------|
| **计算及更新** | VLAN 插入/剥离、Checksum、PTP、Tunnel | RX/TX 均有 | `rte_eth_dev_info_get()` 查询能力 |
| **分片** | **TSO** (TCP Segmentation Offload) | **发送** | `DEV_TX_OFFLOAD_TCP_TSO` |
| **组包** | **RSC** (Receive Side Coalescing) | **接收** | `DEV_RX_OFFLOAD_TCP_LRO` |

---

### 三、DPDK 中的 offload 配置

```c
/* 查询网卡支持的 offload 能力 */
struct rte_eth_dev_info dev_info;
rte_eth_dev_info_get(port_id, &dev_info);

/* 查看 RX offload 能力 */
if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_IPV4_CKSUM)
    printf("支持 RX IPv4 checksum 验证\n");
if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_VLAN_STRIP)
    printf("支持 RX VLAN 剥离\n");

/* 配置 — 启用 checksum 和 VLAN offload */
struct rte_eth_conf port_conf = {
    .rxmode = {
        .offloads = DEV_RX_OFFLOAD_IPV4_CKSUM | DEV_RX_OFFLOAD_VLAN_STRIP,
    },
    .txmode = {
        .offloads = DEV_TX_OFFLOAD_IPV4_CKSUM | DEV_TX_OFFLOAD_TCP_CKSUM
                  | DEV_TX_OFFLOAD_TCP_TSO,
    },
};
rte_eth_dev_configure(port_id, 1, 1, &port_conf);

/* 发包时 — 设置 mbuf ol_flags 告诉网卡做哪些 offload */
mbuf->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM | PKT_TX_TCP_CKSUM;
mbuf->l2_len = sizeof(struct rte_ether_hdr);   /* 网卡需知道 L2/L3/L4 长度 */
mbuf->l3_len = sizeof(struct rte_ipv4_hdr);
mbuf->l4_len = sizeof(struct rte_tcp_hdr);
/* 网卡在 DMA 读出时自动计算并填入 checksum */
```

**RX 侧 — 网卡验证 checksum 后设置标志：**

```c
/* 收包后检查硬件 checksum 验证结果 */
if (mbuf->ol_flags & PKT_RX_IP_CKSUM_BAD)
    drop_packet(mbuf);  /* IP checksum 错误 */
if (mbuf->ol_flags & PKT_RX_L4_CKSUM_BAD)
    handle_l4_error(mbuf);  /* TCP/UDP checksum 错误 */
/* PKT_RX_IP_CKSUM_GOOD — 硬件验证通过，无需软件再算 */
```

---

### 四、offload 的性能权衡

| offload | CPU 节省 | 延迟影响 | HFT 建议 |
|---------|---------|----------|----------|
| **Checksum (RX)** | ~20-50 cycle/包 | 几乎无 | ✅ 启用 — 释放 CPU |
| **Checksum (TX)** | ~20-50 cycle/包 | 几乎无 | ✅ 启用 |
| **VLAN strip** | ~10 cycle/包 | 无 | ✅ 启用 |
| **TSO** | 大幅（避免分片循环） | 增加单包延迟 | ❌ HFT 不用 — TSO 面向大块吞吐 |
| **LRO/RSC** | 减少中断/轮询次数 | 增加单包延迟 | ❌ HFT 不用 — 合并增加延迟 |

---

### 五、与前几章关系

```
Ch5 软件转发算法 (Hash/LPM)     ← 复杂查表仍多在 CPU
Ch6–7 I/O 与 PMD 路径           ← 描述符、burst
Ch8 RSS / Flow Director         ← 硬件分流
Ch9 硬件 offload（本章）        ← 单包机械运算下放 NIC
```

 [Ch5 模块划分 · 硬件加速](../../chapter-05-packet-forwarding/notes/section-2-网络处理模块划分.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 计算及更新卸载](./section-3-计算及更新功能卸载.md)
