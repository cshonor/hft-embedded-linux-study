## 5. DPDK 软件接口与硬件协同

> 硬件执行 offload，但 **配置与语义** 依赖软件 — 「会用 API」与「开 capability」同样关键

---

### 一、`rte_mbuf` 与 `ol_flags`

包粒度 **64 位 offload 状态** 字段 `ol_flags`：

| 示例 (RX) | 含义 |
|-----------|------|
| `PKT_RX_VLAN_PKT` | 硬件已识别/处理 VLAN |
| `PKT_RX_IP_CKSUM_*` | IP 校验结果（GOOD/BAD/NONE） |

| 示例 (TX) | 含义 |
|-----------|------|
| `PKT_TX_IP_CKSUM` | 请硬件计算 **IP 校验和** |
| `PKT_TX_TCP_CKSUM` / `UDP` | L4 checksum offload |

→ mbuf 布局 [Ch6 §6](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md)

---

### 二、RX vs TX  asymmetry

| | **接收** | **发送** |
|---|---------|---------|
| 检测/执行 | 硬件 **自动** 较多 | 常需软件 **逐包声明** |
| 驱动 | 解析 DD / ol_flags 写 mbuf | 写 **TX 上下文/状态描述符**（经 PCIe） |

**TX Checksum / TSO：** 程序员必须设：

- `ol_flags` 相应 **PKT_TX_*** 位  
- **长度字段：** `l2_len`, `l3_len`, `l4_len`（及 TSO 的 `tso_segsz` 等）  

否则硬件 **缺少边界信息**，offload 失败或错包。

---

### 三、配置流程（概念）

```
1. rte_eth_dev_configure / rx/tx_queue_setup
2. 查询 dev_info.offload_capa — 只开 **网卡支持** 的 feature
3. rte_eth_dev_set_mtu / conf.offloads 等
4. 每包：填 mbuf 指针、len、ol_flags → rte_eth_tx_burst
5. RX：读 ol_flags / ptype 决定软件是否还要校验
```

→ 官方 [Programmer's Guide · Offload](https://doc.dpdk.org/guides/prog_guide/overview.html) · repo [chapter-03-PMD](../chapter-03-PMD与轮询模式.md)

---

### 四、HFT 检查清单

- [ ] `rxmode.offloads` / `txmode.offloads` 与 **实际线缆协议** 一致（UDP vs TCP）  
- [ ] RX **BAD cksum** 包是否 **丢弃**（防脏数据进策略）  
- [ ] PTP offload 与 **系统 PHC** 对时流程  
- [ ] 未使用的 offload **关闭** — 减不可控硬件路径  

---

← [4. TSO / RSC](./section-4-分片与组包卸载.md) · 下一节 [6. 小结与索引](./section-6-小结与索引.md)
