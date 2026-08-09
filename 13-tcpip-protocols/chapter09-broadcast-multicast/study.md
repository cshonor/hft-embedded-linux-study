# 第 9 章：广播与本地组播

> 按书节速记：[9.1](9.1-broadcast-multicast-concept.md) · [9.2](9.2-ipv4-broadcast-address.md) · [9.3](9.3-multicast-mac-mapping.md) · [9.4](9.4-igmp-mld-snooping.md) · [9.5](9.5-igmp-mld-attacks.md) · [9.6](9.6-summary.md) · [9.7 实战](9.7-lan-switch-router-multicast.md) · [QUICKREF §9](../QUICKREF.md)

> 《TCP/IP 详解》卷 1 第 2 版（Stevens & Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch09_broadcast_multicast.md)）  
> **交叉引用**：[ch02 地址与子网语义](../chapter02-ip-address-architecture/study.md#ch02-3) · [ch08 ICMP/MLD](../chapter08-icmpv4-icmpv6/study.md) · [ch10 UDP 与一对多套接字](../chapter10-udp-ip-fragment/study.md) · 自顶向下 [§3.3 UDP](../../top_down/03_transport_layer/study.md#ch3-3)

继 [ch02 单播与编址](../chapter02-ip-address-architecture/study.md) 之后，本章把「**一个发送者 → 多个接收者**」在**同一条链路/子网**上的两种核心机制讲透：**广播（broadcast）** 与 **本地组播（local multicast）**。前者由**二层洪泛 + 主机全体收包**驱动；后者用 **D 类 / IPv6 组播前缀** 表达**逻辑组**，并用 **IGMP / MLD** 在路由器上维护**按需转发的软状态**。

| 机制 | 主要作用层次 | 典型语义 | 成员/范围如何确定 |
|------|----------------|----------|-------------------|
| **广播** | 强依赖 **L2** 洪泛 | 一对**子网内全部**接口 | 无「成员」概念，**所有主机**至少处理到能判定丢弃的层次 |
| **组播** | **L3 组地址** + L2 映射 | 一对**加入该组**的接口 | **IGMP/MLD** 报告 + 路由器**定时查询**刷新 |
| **单播** | L3  destination 唯一 | 一对一 | 路由表；与 [ch05 IP](../chapter05-ip-protocol/study.md) 一致 |

单播强调 [端到端](../chapter01-overview/study.md#ch01-e2e)；广播/组播则在**最后一跳子网**上显著改变「谁必须看这份拷贝」。

---

<a id="ch09-1"></a>

## 9.1 引言：广播 vs 组播

→ 精读：[9.1-broadcast-multicast-concept.md](9.1-broadcast-multicast-concept.md) · [定义](9.1-broadcast-multicast-concept.md#ch09-1-def) · [LAN 区别](9.1-broadcast-multicast-concept.md#ch09-1-lan-diff) · [误区](9.1-broadcast-multicast-concept.md#ch09-1-myth) · [考点](9.1-broadcast-multicast-concept.md#ch09-1-cheat)

**核心**：LAN 里**观感可似**，**本质不同** — 广播=一对**全部**强制收；组播=一对**订户**加组才收。

| | 广播 | 组播 |
|---|------|------|
| MAC | `FF:FF:FF:FF:FF:FF` | `01:00:5E…` |
| 跨路由 | **否** | **可**（PIM） |
| 无 Snooping | 泛洪 | **表现像**广播，主机逻辑仍不同 |

→ 广播详 [9.2](#ch09-2) · 组播段 [9.3](#ch09-3) · IGMP Snooping [9.4](#ch09-4)

### 9.1.x 章节导读与 IPv4 广播（扩展）

### 9.1.1 为什么需要「非单播」

- **发现与配置**：部分旧协议用广播做「子网内喊一嗓子」（如某些引导、旧式名字解析）。  
- **一对多效率（组播）**：同一视频/路由更新流不必对 *N* 个订阅者各发 *N* 份单播（广域部分由 **PIM 等**解决；本章聚焦**链路上的本地组管理**）。  
- **代价**：广播让**无关主机**也为每帧付出中断与协议栈成本；组播用过滤与信令把成本压到**订户**附近。

### 9.1.2 受限广播与定向（子网 directed）广播

| 类型 | IPv4 形式 | 路由器是否转发 | 典型用途 / 风险 |
|------|-----------|----------------|-----------------|
| **受限广播**（limited） | **`255.255.255.255`** | **不转发**（仅限本链路/本主机栈认定的「本地」范围） | 引导阶段「我还不知道自己在哪网段」；需防止被滥用做本地放大 |
| **定向广播**（directed / subnet） | **某子网的「主机位全 1」地址** | **历史上可跨网段转发**；现今设备**默认禁止**转发到该子网 | 曾用于「远程唤醒整个子网」；**Smurf** 放大的温床 → [9.2](9.2-ipv4-broadcast-address.md) · [§9.5](#ch09-5) · [ch08 §8.7](../chapter08-icmpv4-icmpv6/study.md#ch08-7) |

### 9.1.3 定向广播地址计算公式

已知主机 IPv4 地址与其**子网掩码**（或前缀长度），**子网定向广播地址**：

```text
定向广播地址 = IP_address | (~subnet_mask)
```

等价写法：`网络号 | 主机位全 1`。其中 `|` 为按位或，`~` 为掩码按位取反（仅主机位为 1）。

**例**：`128.32.1.14/24`，`mask = 255.255.255.0`  
`~mask = 0.0.0.255` → `128.32.1.14 | 0.0.0.255` = **`128.32.1.255`**（该 /24 的定向广播）。

**例**：`10.1.2.3/28`，掩码 `255.255.255.240`，`~mask = 0.0.0.15` → `10.1.2.3 | 0.0.0.15` = **`10.1.2.15`**。

### 9.1.4 以太网与栈内处理路径

1. 上层（常见为 **UDP**）把目的设为广播地址 → **IPv4** 封装。  
2. **链路层**：以太网广播目的 MAC 固定为 **`FF:FF:FF:FF:FF:FF`**（与是否 directed/limited 的 **IP** 语义独立）。  
3. 子网内**所有网卡**至少会**收下帧** → 中断 → 驱动/stack 上行；往往要到 **L3/L4** 才能判断「是否投递给应用」。

因此：**广播的 CPU 成本高在「每台机器都要碰一下」**，与组播多级过滤对照见 [§9.3](#ch09-3)。

### 9.1.5 IPv6：没有广播（no broadcast）

- **IPv6 取消广播**。不再有「全子网二层洪泛语义」与 **IPv4 式定向广播**。  
- 替代：**链路本地组播**（如 **`ff02::1` 全体节点**、**`ff02::2` 全体路由器**）与 **ND**（邻居发现，`NS/NA` 走 ICMPv6，常配合**被请求节点多播地址**而非全网广播）。  
- 纵深阅读：[ch02 IPv6 与多播前缀](../chapter02-ip-address-architecture/study.md#ch02-3) · [ch08 ND](../chapter08-icmpv4-icmpv6/study.md#ch08-5)

### 9.1.6 工程设计要点（与 ch08/ch07 联动）

- **路由器/三层交换机**：常用 **`no ip directed-broadcast`** 一类配置**禁止定向广播穿透路由**（默认行为依实现而异，考试时以「**现代默认审慎**」为准）。  
- **主机**：对入向广播 ICMP/UDP **限速与访问控制**。  
→ 防火墙与边界策略：[ch07](../chapter07-firewall-nat/study.md#ch07-7)。

---

<a id="ch09-2"></a>

## 9.2 广播（Broadcast）

→ 精读：[9.2-ipv4-broadcast-address.md](9.2-ipv4-broadcast-address.md) · [受限](9.2-ipv4-broadcast-address.md#ch09-2-limited) · [定向](9.2-ipv4-broadcast-address.md#ch09-2-directed) · [IPv6](9.2-ipv4-broadcast-address.md#ch09-2-v6) · [考点](9.2-ipv4-broadcast-address.md#ch09-2-cheat)

| 类型 | 地址 | 转发 |
|------|------|------|
| **受限广播** | **`255.255.255.255`** | **不跨路由** → DHCP/ARP |
| **定向广播** | 主机位全 1（如 `192.168.1.255`） | **默认丢弃** → 防 Smurf |

**IPv6 无广播** → **`FF02::1`**（所有节点）、**`FF02::1:FF…`**（替代 ARP）。  
广播危害：二层泛洪 → 全员处理 → **风暴**。

→ 定向广播公式、9.1 导读详述仍见 [§9.1.2–5](#ch09-1)

---

<a id="ch09-2-bridge"></a>

## 9.2+ 分发模型鸟瞰：为何要「多级过滤」

| 关注点 | 广播 | 局域网组播（本章重点） |
|--------|------|-------------------------|
| **状态** | 无成员表（谁都要听二层洪泛一遍） | 路由器/交换机可维护 **ASM 下的组订阅/接口状态**一类 **软状态**（IGMP/MLD） |
| **广域延展** | 基本停在本子网 | 域内尚需 **PIM-SM/PIM-SSM** 等（卷一仅点到为止） |
| **与 UDP** | ICMP/引导类常见 | **`SO_REUSEADDR`/`SO_REUSEPORT`** + **JoinGroup**：见 [ch10](../chapter10-udp-ip-fragment/study.md) |

本节为 [§9.3](#ch09-3) 的 **`01:00:5E`/`32:1`** 推导做铺垫：**二层只能粗略筛**，**三层组播 IP 才是最终判决**。

---

<a id="ch09-3"></a>

## 9.3 组播基本概念与二层映射（IPv4 为重点）

→ **MAC / IPv4 单播·组播·广播对照 + PAUSE 与 IP 组播区别**：[9.3 精读](9.3-multicast-mac-mapping.md) · [**组播地址段/传播**](9.3-multicast-mac-mapping.md#ch09-3-scope)

### 9.3.1 IPv4 组播地址范围与传播

**核心**：**224.0.0.0/4** — 大部分**默认仅 LAN**；跨网段需 **PIM**。

| 段 | 传播 |
|----|------|
| **`224.0.0.0/24`** | **不跨路由** — `224.0.0.1`/`224.0.0.5` OSPF 等 |
| **`224.0.1.0/24`** | 默认可跨网/公网 |
| **`224.0.2.0`~`238.x`** | 需 **PIM**；家用默认不转发 |
| **`239.0.0.0/8`** | **私有组播**，不进公网 |

→ 详：[9.3 §传播范围](9.3-multicast-mac-mapping.md#ch09-3-scope) · [考点](9.3-multicast-mac-mapping.md#ch09-3-cheat)

### 9.3.2 IPv4 → 以太网 MAC：IANA `01:00:5E` 与「丢 9 位」

IPv4 **组播 IP** 的低 **23 bit** 直接映射进以太网组播 MAC 的低 **23 bit**；高 **9 bit**（在 D 类里对应 **中间 9 bit**，即组 ID 的 **位 23–31**）在映射中**不参与**以太网 MAC 区分。

```text
IPv4  multicast: 1110xxxx  xxxxxxxxx  xxxxxxxxxxxxxxxxxxxxxxxxxxxx
以太网 multicast:      01-00-5E | 0 | xxxxxxxxxxxxxxxxxxxxxxx （23 bits）
                             ↑ IEEE 预留，且该位通常为 0，使 MAC 落入 01-00-5E-00-xx-xx～01-00-5E-7F-xx-xx
```

（具体位图以教材图为准：**28 位可变的组标识**里只有 **23 位**进入 MAC，**5 位被折叠**。）

### 9.3.3 为什么是 32:1 映射（同一 MAC：32 个 IP）

- IPv4 **组地址中参与 MAC 计算的「自由选择位」**：**28 位**（D 类的组 ID 主体）减掉映射进 MAC 的 **23 位** → 剩余 **5 位**。  
- **2⁵ = 32**：即 **至多 32 个不同 IPv4 组播地址**会映射到**同一个**以太网组播 MAC。  
- **工程后果**：单靠网卡/MC 哈希**无法区分**这 32 个组 → 必须在 **L3（IP）再做一次精确过滤**。

### 9.3.4 「三级过滤」减轻主机 CPU（必背）

为避免每个组播以太网帧都打断所有主机：

| 层级 | 做什么 | 仍可能漏过什么 |
|------|--------|----------------|
| **1. 硬件/驱动**（理想） | 网卡仅以 **若干组播 MAC 过滤器** 接收感兴趣的 L2 组播（或哈希不完美时适度放宽） | **哈希冲突**：无关 MAC 仍偶发入内 |
| **2. 设备驱动 / L2** | 以 **CRC/目的 MAC** 丢弃明显无关帧（实现相关） | 无法解决 **32:1**：目的 MAC 「看起来相关」但仍可能不是本主机要的 **IP 组** |
| **3. 网络层（IP）** | 仅在 **完整组播 IP 目的地址**匹配本机订阅的 **IGMP Join**（或内核策略）后继续上传 | 「最后一道真理」 |

**一句话**：**L2** 减负，**L3** 裁决；考试常考「**为何 IP 映射到 MAC 后还要再过滤**」。

### 9.3.5 IPv6 组播（与 MLD）

- IPv6 **组播前缀**：**`ff00::/8`**；链路范围常见 **`ff02::/16`**（如 **路由器/主机组**）。  
- **无广播** → ND、DHCPv6 **都用 ICMPv6/UDP + 链路组播**。  
→ MLD 报文装在 **ICMPv6**（Type 查询/监听报告类）——与 [ch08 §8.2](../chapter08-icmpv4-icmpv6/study.md#ch08-2) 同属 **ICMPv6 大家族**。

---

<a id="ch09-4"></a>

## 9.4 IGMP / MLD：加入组播组

→ 精读：[9.4-igmp-mld-snooping.md](9.4-igmp-mld-snooping.md) · [原理](9.4-igmp-mld-snooping.md#ch09-4-join-principle) · [Win/Linux](9.4-igmp-mld-snooping.md#ch09-4-join-linux) · [程序](9.4-igmp-mld-snooping.md#ch09-4-join-code) · [考点](9.4-igmp-mld-snooping.md#ch09-4-cheat)

**一句**：**加组 = 告诉系统/网卡要收该组流量** → 内核发 **IGMP Join** → 路由器记录。

| 方式 | 命令/API |
|------|----------|
| **Windows** | `netsh interface ipv4 add joinmulticastgroup …` |
| **Linux** | `ip maddr add 239.x.x.x dev eth0` |
| **程序** | **`IP_ADD_MEMBERSHIP`** → 自动 IGMP Report |

**vs 广播**：组播**只有加组才收**；广播**强制全员**。

### 9.4.x IGMP 软状态、版本与 Snooping（扩展）

### 9.4.1 为何要 IGMP / MLD

- **路由器**需要知道：**这条链路上，是否还有人要某某组播 `(G)` 或 `(S,G)`**？  
- 离开组时必须**滞后更新**太慢会浪费链路带宽；报告太猛会抖动控制面。**IGMP / MLD** 在「**稀疏成员**」「**交换机 snooping**」环境里尤其关键。

### 9.4.2 软状态（soft state）

- 路由器（或 IGMP proxy）保存的 `(接口 → 感兴趣的组)` **不是永久性配置真理**，而是由 **周期性查询（Query）+ 主机报告（Report）** **不断刷新**。  
- **超时**未听到某组的「还活着」的证明 → **删除状态**，停止向该接口转发对应组流量。  
- **对比硬状态**：若只靠静态配置写入，移动与即插即用会极难运维；但软状态也意味着**可被伪造** → [§9.5](#ch09-5)。

### 9.4.3 IGMP 各版本对照表（精简必背）

（报文编号与语义以 Stevens/Fall **卷 1 ed2·第 9 章叙述**为根本；RFC 演进：v2、v3 为主流。）

| 特性 | IGMPv1 | IGMPv2 | IGMPv3 |
|------|--------|--------|--------|
| **查询者（Querier）** | 每条二层网段选出 **Active Querier**，由其**周期性发送** General Query | 同左；新增 **Group-Specific Query**（Leave 后快速确认成员） | 再增 **源特定**查询语义（与 `(S,G)` 状态对齐） |
| **成员报告 Membership Report** | 报告 **组地址 G** | 仍为 **报告 G**，但语义与 RFC2236 对齐；与离组交互更好 | **IGMPv3 Membership Report**：可携带 **源列表/filter** mode（含 **INCLUDE/EXCLUDE**），表达 **对一个组内源的集合兴趣** → **SSM `(S,G)`** 地基 |
| **离组 Leave** | **无**：靠**静默** + `group membership interval` **超时剔除** → 慢 | **Leave Group** 报文触发 **specific group query**，快速删掉无人听的组 | 由 **兼容 v3 的 state change report**（实现细节依栈）细化源级状态 |
| **通用查询 General Query** | 有，`Max Response Time` 控制随机延迟上限 | Max Response Time 可配；查询间隔、健壮性可调 | 仍可发 General Query，但主机侧行为更精细 |
| **报告抑制 Report Suppression（旧版语义）** | **有**：倒计时期间若听见**同组的他人报告**，可**抑制发送**以降低突发报告 | IGMPv2 仍沿袭「可抑制」以降低洪泛 **（实现可偏离）** | **不适用/显著弱化**：**SSM/v3** 常为**每台主机独立源过滤状态**，**无法用「别人替我报告」等价代替**——**不能与 v1/v2 的抑制模型硬套同一逻辑** |

**记忆钩子**：

- **v1**：慢离开；模型最简单。  
- **v2**：**快速离组**，运营友好。  
- **v3**：**按源订阅** → **特定源组播 SSM**，与 **ASM（任意源 `*,G`）** 分流。

### 9.4.4 查询者选举（Querier）

- 同一二层网段应有**唯一的 Active Querier**，由其**周期性**发送 Query（多台路由器连接同一广播域时）。  
- 常见选举规则：**IPv4 地址最小者胜出**（实现细节可查所用协议栈/quirks；考试记「**竞选出唯一查询者」**）。

### 9.4.5 MLD / MLDv2（IPv6）

| 类比 | ICMPv6 侧 | Notes |
|------|-----------|-------|
| 角色 | **MLDv1 ↔ IGMPv2**（离开、特定组语义增强）· **MLDv2 ↔ IGMPv3（源筛选）** | 报文在 **ICMPv6** |
| Query / Report | Listener Query / Listener Report … | ND 不使用 IGMP |

**关键点**：别把 **ARP** 和 **ND**、**MLD** 混为一谈：**MLD ≠ ND**。

---

<a id="ch09-5"></a>

## 9.5 安全：广播放大与 IGMP / MLD 控制面作恶

广播与 IGMP **都没有内建真实性**；二层域若「太宽」，更易出事。

### 9.5.1 Smurf（与定向广播的结合）

**机制概要**：

1. 攻击者向**某子网的定向广播地址**发起大量 **ICMP Echo Request（ping）**。  
2. **源 IPv4** 设为**受害者**。  
3. 子网内**大量主机同时向伪造源回应 Echo Reply**，形成对受害者的 **N:1** 放大。

### 9.5.2 流氓成员报告（Rogue Membership Reports）

攻击者海量发送 **Membership Report / MLD Listener Report**，声称对某些组有兴趣：

| 直接影响 | 说明 |
|---------|------|
| **路由器/交换芯片状态耗尽** | 组表项暴增 |
| **错向转发** | 不该来的组播被推送到段内 |
| **与 Snooping 联动紊乱** | 端口被错误地学习为「听者」端口 |

缓解：**组播过滤**（接入侧）、**每端口/每台设备组数量上限**、**802.1X 等认证接入**、**Storm Control**。

### 9.5.3 伪造低地址抢 Querier（Querier 欺骗）

- 若能投 **虚假的 Query**（源 IP **很小**以满足选举），可把 **Active Querier** 「顶掉」或打乱查询节奏。  
- 后果：**成员超时错误**、**状态振荡**或**可被利用的流量可见性操纵**。

### 9.5.4 纵深防御小结

| 层级 | 措施 |
|------|------|
| **路由侧** | 禁止不必要 **directed broadcast forwarding** |
| **交换侧** | **IGMP/MLD Snooping**、未知组丢弃、可控 **flooding** 模式 |
| **主机侧** | 默认别加入宽泛组播；防火墙限制入向 ICMP/UDP |
| **与 ICMP 攻防关联** | [ch08 §8.7 ICMP 攻防](../chapter08-icmpv4-icmpv6/study.md#ch08-7) |

---

<a id="ch09-7"></a>

## 9.7 实战：交换机、路由器与组播转发

→ 精读：[9.7-lan-switch-router-multicast.md](9.7-lan-switch-router-multicast.md) · [交换机/网关](9.7-lan-switch-router-multicast.md#ch09-7-switch-router) · [Snooping](9.7-lan-switch-router-multicast.md#ch09-7-igmp-snooping) · [IP→MAC 流程](9.7-lan-switch-router-multicast.md#ch09-7-ip-mac)

| 设备 | 层次 | 组播相关 |
|------|------|----------|
| **交换机** | L2 / MAC | 无 Snooping → 组播**泛洪**；有 Snooping → **精准端口** |
| **路由器/三层** | L3 / IP | **IGMP** 记录谁订阅哪个组 |

**同网段** → 交换机，**不经网关**；**224/x → 01:00:5E**；主机**加组才收**。

---

<a id="ch09-exam"></a>

## 9.6 总结与考点

### 9.6.1 单播 vs 广播 vs 组播（对比总表）

| 维度 | 单播（Unicast） | 广播（Broadcast） | 组播（Multicast） |
|------|----------------|-------------------|-------------------|
| **L3 目的语义** | 唯一主机/接口地址 | 「本子网全员」：`255.255.255.255` 受限；或为**子网广播**主机位全 1 | **一组订阅者**：IPv4 **`224/4`** · IPv6 **`ff00::/8`** |
| **L2（以太网）典型目的** | 学习到的单播 MAC | **`FF:FF:FF:FF:FF:FF`** | **`01:00:5E`** 前缀 + IP 的低 **23bit** |
| **谁必须处理拷贝** | 路径上仅收件人（理想） | 子网**全部主机**至少要处理接收 | **已加入组的节点**主导；多级过滤可减少无关 CPU |
| **IPv6** | ✅ | **无IPv6广播概念** — 用链路组播取代 | ✅（主流通路） |
| **传输层实务** | TCP/UDP 均可（TCP **仅点对点**语义） | 常见 UDP/ICMP；TCP **无意义**（会话需点对点子网可达） | 常见 **UDP**；见 [**ch10 UDP**](../chapter10-udp-ip-fragment/study.md)：组播 Join、TTL/跳数限制 |
| **核心控制协议** | ARP/RARP **非**本节主题；单播路由即够 | （无 IGMP）；靠 **子网 + 链路洪泛语义**定义 | **IGMP / MLD**，软状态 |

### 易混速记

| 题眼 | 正解要点 |
|------|-----------|
| 受限广播 IP | **`255.255.255.255`**，路由器**不转发** |
| **定向广播**怎么算 | `定向广播 IP = IPv4 \| (~subnet_mask)`（等价：网络号 + 主机位全 1） |
| IPv6 「广播 Ping」为何不存在 | IPv6 **无广播** → 用 **`ff02::1`**（全体节点链路组播 ICMPv6 Echo）一类替代 |
| 广播类型 | **受限 255.255.255.255** vs **定向 x.x.x.255** → [9.2](9.2-ipv4-broadcast-address.md#ch09-2-cheat) |
| 广播 vs 组播 v6 | v6 **取消广播**；**FF02::1** 替代子网广播 → [9.2 §IPv6](9.2-ipv4-broadcast-address.md#ch09-2-v6) |
| 组播地址传播 | **224.0.0/24 不跨路由**；**239.x 私有**；跨网需 **PIM** → [9.3 §传播](9.3-multicast-mac-mapping.md#ch09-3-scope) |
| 广播 vs 组播 LAN | 全员强制 vs 加组才收；无 Snooping **表现像**但逻辑不同 → [9.1](9.1-broadcast-multicast-concept.md#ch09-1-compare) |
| 加入组播组 | **IGMP Join** / **`IP_ADD_MEMBERSHIP`** → [9.4](9.4-igmp-mld-snooping.md#ch09-4-join-principle) |
| 交换机 vs 路由器 | 同网段**交换机MAC**；跨网段**网关IP**；**IGMP**三层 · **Snooping**二层 → [9.7](9.7-lan-switch-router-multicast.md#ch09-7-cheat) |
| 32:1 | **32** 个不同 **IPv4 组播 IP**→ 可能共享**同一以太网 MAC** → **三层再过滤** |
| 三层过滤的顺序 | **硬件/MC 哈希 →（驱动）二层目的筛选 → IP 订阅匹配** |
| **报告抑制 vs IGMPv3** | **v1/v2** 可降低重复报告 **；v3/SSM 源状态个人化一般不靠「同人代报」** |
| IGMP ↔ MLD | **IGMP**：IPv4；**MLD**：IPv6 ICMPv6 承载 |

### 推荐阅读顺序

1. **编址与子网**：子网边界如何定义「本子网」「广播给谁」—— [ch02 特殊地址与编址](../chapter02-ip-address-architecture/study.md#ch02-3)。  
2. **ICMP / MLD / 攻击面**：回声、ND、Smurf—— [ch08](../chapter08-icmpv4-icmpv6/study.md)。  
3. **套接字与 UDP 工程**：`IP_ADD_MEMBERSHIP`、`IP_MULTICAST_TTL`、[ch10](../chapter10-udp-ip-fragment/study.md)。

### 下一章

- **[ch10 UDP](../chapter10-udp-ip-fragment/study.md)** — 组播 socket、跳数限制  
- **[ch08 ICMP](../chapter08-icmpv4-icmpv6/study.md)** — MLD  
- **[ch07 防火墙](../chapter07-firewall-nat/study.md#ch07-7)** — 过滤与阈值

---

## Top-Down

- [§3.3 UDP](../../top_down/03_transport_layer/study.md#ch3-3) · [考点](../../top_down/03_transport_layer/study.md#ch3-3-exam)（广播域、组播能力与 socket API）  
- [链路层与 LAN · VLAN/广播域](../../top_down/06_link_layer_and_lan/study.md#ch6-4)  
- 组播路由（PIM 等）常归入控制面笔记本：[routing / control-plane](../../top_down/05_network_layer_control_plane/study.md)

## Lab

- **IPv4 广播**：`ping 255.255.255.255`（受限）与子网 **`ping x.x.x.255`/定向广播`**（需在 OS 与安全策略允许时实验）。  
- **抓包**：`tcpdump igmp`、`icmp and ip`；IPv6：`tcpdump icmp6 AND ip6`。  
- **本机**：`netstat -g` / **`ip maddr`** / `ip igmp`。

## Go / Rust

- **Go**：`ipv4.PacketConn` / `syscall`：`JoinGroup`、`SetMulticastInterface`、`SetTTL`；注意 **监听地址为 `ANY` vs 显式组**。  
- **Rust**：`socket2`/`tokio` 下 `setsockopt`、`IP_MULTICAST_IF`、`IP_ADD_MEMBERSHIP`；常与 **reuse** 语义一起考。  
- **排障清单**：交换机 **IGMP snooping** 是否误裁剪；宿主桥接/`vswitch`「未知组洪泛 vs 丢弃」；(S,G)/(,G) 是否搞反。
