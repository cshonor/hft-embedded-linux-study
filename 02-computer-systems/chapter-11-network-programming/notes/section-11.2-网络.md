## 11.2 网络

> **Ch11 §11.2** · [章导读](../README.md) · 上节 [§11.1 ←](./section-11.1-客户端-服务器编程模型.md) · 下节 [§11.3 →](./section-11.3-全球IP互联网.md)

---

- **LAN / WAN** — 以太网、交换机、路由器
- **协议分层** — 应用 / 传输 / 网络 / 链路（细节 → [13-TCP-IP](../../../13-tcpip-protocols/)）

---

### 网络基础概念

- **LAN（局域网）** — 以太网、交换机；同一广播域
- **WAN（广域网）** — 路由器互联多个 LAN；互联网是最大的 WAN
- **协议分层（TCP/IP 四层模型）：**
  - **应用层** — HTTP、FIX、自定义二进制协议
  - **传输层** — TCP（可靠）/ UDP（低延迟）
  - **网络层** — IP（路由、寻址）
  - **链路层** — 以太网帧、ARP

| 层 | 数据单元 | HFT 关注 |
|----|----------|----------|
| 应用 | message | 协议解析效率 |
| 传输 | segment/datagram | TCP vs UDP 选择 |
| 网络 | packet | 路由跳数、MTU |
| 链路 | frame | 交换机延迟、NIC offload |

### 常见陷阱
1. **协议分层不是为了慢，是为了解耦** — 每层只管自己的职责，上层不关心下层细节
2. **HFT 关注所有层的延迟** — 链路层（NIC + 交换机）、网络层（路由跳数）、传输层（TCP 握手/Nagle）、应用层（解析）
3. **以太网帧 MTU=1500** — 大于 MTU 的 IP 包分片，分片增加延迟和重组开销

### 自测题

<details>
<summary>Q1: TCP/IP 四层模型分别是什么？每层的数据单元叫什么？</summary>

应用层（message）、传输层（segment/datagram）、网络层（packet）、链路层（frame）。

</details>

<details>
<summary>Q2: LAN 和 WAN 的区别？交换机和路由器分别在哪个层？</summary>

LAN 是局域网（同一广播域），用交换机（链路层）互联。WAN 是广域网，用路由器（网络层）互联多个 LAN。

</details>

<details>
<summary>Q3: HFT 在网络延迟方面关注哪些因素？</summary>

链路层：NIC 延迟、交换机跳数、ARP 缓存。网络层：路由跳数、MTU 大小。传输层：TCP vs UDP、Nagle 算法。应用层：协议解析效率。

</details>

<details>
<summary>Q4: 以太网 MTU 是多少？超过会怎样？HFT 如何避免？</summary>

MTU=1500 字节。超过则 IP 分片，增加延迟和重组开销。HFT 避免分片：控制包大小 < MTU，或用 UDP 而非 TCP（避免 MSS 协商）。

</details>

---

← [§11.1 ←](./section-11.1-客户端-服务器编程模型.md) · [本章导读](../README.md) · [§11.3 →](./section-11.3-全球IP互联网.md)
