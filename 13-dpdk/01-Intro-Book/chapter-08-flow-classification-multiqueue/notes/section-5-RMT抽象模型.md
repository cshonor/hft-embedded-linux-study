## 5. 抽象模型：RMT（可重构匹配表）

> 网卡流分类 ≈ SDN 的 **Match + Action**

---

### 一、RMT 是什么

**RMT (Reconfigurable Match Tables)** — 软件自定义网络中的抽象：

```
匹配 (Match)  — 报文字段 / 掩码 / 优先级
    ↓
动作 (Action) — 分配队列、丢弃、打标签、计数 …
```

要点是"**可重构**"：匹配哪些字段、做什么动作，不再烧死在芯片里，
而是由软件在运行时下发——这是从固定功能网卡走向可编程数据面的分水岭。

---

### 二、各类技术 = 不同 Match/Action

| 技术 | Match（简） | Action（简） | rte_flow 对应 |
|------|-------------|--------------|---------------|
| **RSS** | 哈希关键字（四元组等） | **散列到 queue** | `RTE_FLOW_ACTION_TYPE_RSS` |
| **Flow Director** | **精确** 五元组/自定义 | **导向指定 queue** | pattern + `..._QUEUE` |
| **QoS** | VLAN UP / TC | **优先级调度** | `..._MARK` / `..._PF`/`VF` |
| **包类型识别** | 帧格式解析 | **描述符写 ptype** | （硬件自动，非流表） |

**统一视角：** 区别仅在 **匹配字段** 与 **动作语义** — 便于理解 **新一代智能网卡** 能力边界。

---

### 三、DPDK 的落地：rte_flow 就是 RMT 接口

`rte_flow_*` API 把 RMT 思想变成了可移植代码——
**pattern 数组描述 Match，action 数组描述 Action**，都由 `END` 收尾：

```c
/* Match: IPv4 + UDP 且 dst_port == 10001 → Action: 进 queue 3 + 计数 */
struct rte_flow_item pattern[] = {
    { .type = RTE_FLOW_ITEM_TYPE_ETH },
    { .type = RTE_FLOW_ITEM_TYPE_IPV4 },
    { .type = RTE_FLOW_ITEM_TYPE_UDP,
      .spec = &(struct rte_flow_item_udp){
              .hdr.dst_port = rte_cpu_to_be_16(10001) },
      .mask = &(struct rte_flow_item_udp){
              .hdr.dst_port = 0xffff } },        /* 掩码=精确匹配这个端口 */
    { .type = RTE_FLOW_ITEM_TYPE_END },
};

struct rte_flow_action actions[] = {
    { .type = RTE_FLOW_ACTION_TYPE_QUEUE,
      .conf = &(struct rte_flow_action_queue){ .index = 3 } },
    { .type = RTE_FLOW_ACTION_TYPE_COUNT },      /* 顺手让硬件计数，免软件统计 */
    { .type = RTE_FLOW_ACTION_TYPE_END },
};

struct rte_flow_attr attr = { .ingress = 1, .priority = 0 };
struct rte_flow_error err;
struct rte_flow *flow =
    rte_flow_create(port, &attr, pattern, actions, &err);
/* 先 rte_flow_validate() 试错是常规做法：不同网卡支持的 item/action 组合差异很大 */
```

三个工程要点：

1. **可移植性是"查询出来"的**：网卡能力差异极大，下发前先 `rte_flow_validate()`，
   不支持就降级（如 COUNT→软件计数）。写死一套规则换卡必翻车。
2. **priority 决定匹配顺序**：多条规则可能同时命中时，数值小的先匹配——
   "丢弃特定垃圾流"的 DROP 规则要给比"放行同类流"更高的优先级。
3. **每条硬件规则都有创建成本**（µs~ms 级，要写网卡寄存器/TCAM）：
   规则在**启动时一次下发**，别在数据面热路径上增删——
   行情频道动态订阅的场景，频道→队列的映射变化应走 RETA 更新或软件层。

---

### 四、趋势：从"可配置"到"可编程"

```
固定功能网卡        →  可配置（RSS/FD/QoS，本节）  →  可编程数据面
（match/action 烧死）   （字段可选，动作可选）           （P4 程序、DPU 上的 ARM 核）
```

- **P4 / 可编程交换芯片**（Tofino 系）：Match/Action 表本身由 P4 程序定义，
  RMT 的完全体——HFT 里的对应物是 Arista 7130 这类带 FPGA 的交换机，
  把订单编码、风控前置做到**线速几百 ns**，直接越过主机。
- **DPU / SmartNIC**（BlueField、IPU）：网卡上带通用核，
  把策略的一部分下推过去——收益是解放主机核、路径更短；
  代价是**调试链条剧增**（网卡里的代码不在你的 perf/gdb 视野里），
  且订单路径多一跳 PCIe 之前要先想清楚延迟账。

HFT：**硬件 RSS+FD** 把 **fan-out** 做在 NIC；策略核专注 **解码与交易逻辑**。
再往下推（FPGA/DPU）是"用工程复杂度换最后几百 ns"——
只有在主机路径已被 [ch07](../../chapter-07-nic-performance-optimization/) 的方法榨干之后才值得考虑。

---

← [4. 实战](./section-4-DPDK实战结合.md) · 下一节 [6. 索引](./section-6-小结与索引.md)
