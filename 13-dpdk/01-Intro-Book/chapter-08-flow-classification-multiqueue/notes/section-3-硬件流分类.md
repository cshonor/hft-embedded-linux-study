## 3. 硬件流分类 (Flow Classification)

> 网卡 **硬件** 识别流 → 写描述符 / **导向队列** — 卸载 CPU 解析

---

### 一、流分类是什么

网卡依据包 **特性** 分类，将结果：

- 记录在 **RX 描述符** 中，或
- **直接** 把流导入 **特定队列**

DPDK 通过 **mbuf** 读标志 — **免软件 parse 包头**。

本质是把软件里这段每个包都要跑的判断：

```c
/* 软件路径：逐层解封装，每包 ~几十~上百 cycles */
eth = rte_pktmbuf_mtod(m, void *);
if (eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
    ip = (void *)(eth + 1);
    if (ip->next_proto_id == IPPROTO_UDP) { /* ... */ }
}
```

变成**读一个硬件已经写好的字段**——下面第二节。

---

### 二、包类型识别（Packet Type Offload）

高级网卡（如 **Intel XL710/E810**）在硬件识别：

- **L3/L4**：IPv4/IPv6、TCP、UDP
- **隧道**：VXLAN、NVGRE、GRE 等（内外层各给一套）

信息挂在 **接收描述符**，DPDK 映射为 `mbuf->packet_type`：

```c
/* 硬件路径：一次掩码判断，无指针跳转、无分支级联 */
uint32_t pt = m->packet_type;

if ((pt & RTE_PTYPE_L3_MASK) == RTE_PTYPE_L3_IPV4 &&
    (pt & RTE_PTYPE_L4_MASK) == RTE_PTYPE_L4_UDP) {
    /* 直接按已知偏移取 UDP 头，跳过 ether_type/proto 的逐层判断 */
}

/* 隧道场景还能一次问出内层协议： */
if ((pt & RTE_PTYPE_TUNNEL_MASK) == RTE_PTYPE_TUNNEL_VXLAN) { /* ... */ }
```

收益不只是省 cycles：**分支更可预测**。软件逐层判断的级联 if 在混合流量下
分支预测失败率高；掩码比较是无分支的，流水线更顺——
这正是 [ch03 ILP 一节](../../chapter-03-parallel-computing/notes/section-3-指令级并发.md)
讲的"把控制相关转成数据相关"。

---

### 三、RSS（Receive Side Scaling）— 负载均衡

| 步骤 | 说明 |
|------|------|
| **关键字** | 常取 **四元组**（IP + 端口）等，由 `rss_hf` 位掩码选择 |
| **哈希** | **Toeplitz** 算法 + 40 字节随机密钥 → 32 位 hash |
| **散列** | hash 低 7 位查 **RETA**（重定向表，128 项）→ RX queue |

注意散列**不是直接模队列数**——中间隔着 RETA 表，这意味着：

```c
/* RETA 可逐队列改权重：让 queue 0 承担 2 倍流量 */
struct rte_eth_rss_reta_entry64 reta_conf[2];  /* 128 项 = 2 组 64 */
/* ... 填表：奇数项→queue0，偶数项→queue1，2:1 分流 ... */
rte_eth_dev_rss_reta_update(port, reta_conf, 128);

/* 选择参与哈希的字段：行情 UDP 按四元组 */
struct rte_eth_rss_conf rss = {
    .rss_hf = RTE_ETH_RSS_IPV4 | RTE_ETH_RSS_UDP,   /* src/dst IP + src/dst port */
    .rss_key = NULL,   /* NULL = 用网卡默认密钥；多机同行为要显式固定 */
};
rte_eth_dev_rss_hash_update(port, &rss);
```

**对称哈希 (Symmetric Hash)：**
正向与反向流（A→B 与 B→A）→ **同一队列 / 同一核** — **防火墙、会话表** 必需。
Intel 卡可通过修改 RSS 密钥（交换输入字节序的特制法）或对 TCP 用
`RTE_ETH_RSS_SYMMETRIC_XOR`（新驱动）实现；实现不了就退化到 Flow Director 逐会话定向。

⚠ **HFT 的坑（[§2 已详述](./section-2-网卡多队列.md)）：** 组播行情**固定五元组**，
RSS hash 后全部落进**同一个队列**——RSS 对组播粉丝扇出场景**完全无效**，
分流只能靠 Flow Director 按组播组+端口逐条定向。

**软件兜底：** 老网卡/虚拟网卡不支持 RSS 时，DPDK 提供软件 RSS
（`rte_thash` 同款 Toeplitz 在 PMD 内算），代价是哈希计算回到 CPU。

---

### 四、Flow Director — 精确匹配

Intel **精确匹配** 技术：

- 网卡内 **匹配表** — 按 **特定 TCP 会话** 等字段（XL710 约 8K 条 perfect-match）
- 命中 → **导向预留的特定队列**（也能打标、丢弃）

DPDK 里新旧两套接口：

| 接口 | 状态 | 说明 |
|------|------|------|
| `rte_eth_dev_filter_ctrl()` + `RTE_ETH_FILTER_FDIR` | 旧版/废弃 | 早期 ixgbe/i40e 专用 |
| **`rte_flow_*`** | **现行标准** | 统一 Match/Action 模型，见 [§5 RMT](./section-5-RMT抽象模型.md) |

**用途：** 少量 **控制流 / 会话** 从海量转发流量中 **硬件剥离** — CPU **零过滤**。
HFT 典型配置（组播组 224.1.0.1:10001 → queue 3 的完整代码）见
[§2 Flow Director 一节](./section-2-网卡多队列.md)。

表项是**稀缺资源**：8K 条对逐会话 TCP 定向够用，
想按"每个 symbol 一条规则"扇出万级行情频道就超了——
那种规模要么归并成掩码规则，要么回到软件分发。

---

### 五、QoS（服务质量）

依据 **VLAN UP（用户优先级）** 等：

- 划分 **Traffic Class (TC)**
- TC 间 **Weighted Strict Priority** 等 **硬件调度** — 保障高优先级带宽

HFT：共置机 **裸金属** 较少用 VLAN QoS；**托管/NFV** 场景更常见。
但有一个变体值得知道：**行情和订单共网卡**时，用 TC 把订单标高优先级，
避免行情突发挤占订单出口带宽——交易所共置网段内这通常是交换机侧配置，
主机侧只需打 VLAN/DSCP 标记。

---

← [2. 多队列](./section-2-网卡多队列.md) · 下一节 [4. 实战](./section-4-DPDK实战结合.md)
