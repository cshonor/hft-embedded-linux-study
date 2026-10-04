# Ch 13 DPDK 与 NFV · DPDK & Network Function Virtualization

> **01-Intro-Book** · 《深入浅出 DPDK》第十三章 · **🟡 应用篇**  
> **第三部分「DPDK 应用篇」开篇** — NFV 架构、OPNFV、VNF 优化与商业案例

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. NFV 起源与架构 | [notes/section-2-NFV起源与架构.md](./notes/section-2-NFV起源与架构.md) |
| 3. OPNFV 与扩展 API | [notes/section-3-OPNFV与DPDK扩展API.md](./notes/section-3-OPNFV与DPDK扩展API.md) |
| 4. VNF 评估与性能分析 | [notes/section-4-VNF评估与性能分析.md](./notes/section-4-VNF评估与性能分析.md) |
| 5. VNF 深度优化设计 | [notes/section-5-VNF深度优化设计.md](./notes/section-5-VNF深度优化设计.md) |
| 6. 实例与小结 | [notes/section-6-实例与小结.md](./notes/section-6-实例与小结.md) |

---

## HFT 视角：NFV 是反面教材，也是延迟预算的说明书

NFV 的方向与 HFT **相反**：NFV 用通用服务器+虚拟化替代专用硬件（要弹性、要成本），
HFT 用专用化替代通用性（要确定性）。所以本章对 HFT 的价值不在"怎么用"，而在两个"认清"：

1. **认清 VNF 链的延迟结构**：VNF 串链 = 每个环节一次虚拟交换+一次可能的 VM 进出，
   延迟**逐级累加**——这和 [Ch5 Pipeline](../chapter-05-packet-forwarding/) 的
   "每级 stage 队头等待累加"是同一个规律，只是 NFV 把它放大到了 µs 级每级。
   看完你就理解为什么交易路径**一个 VM 都不串**。
2. **认清对端在 NFV 化**：交易所/券商/运营商的网关、风控前置越来越多跑在
   NFV/OVS-DPDK 栈上——**你的订单路径延迟预算里有他们的一份**。
   读 [Ch4 VNF 评估](./notes/section-4-VNF评估与性能分析.md) 的方法（分解每级开销），
   同样适用于向券商追问"你们网关这 2µs 花在哪"。

**一句话：** 自己不部署 NFV，但要会算 NFV 的账——它是你和对端谈延迟时的共同语言。

---

## 相关

- 上一章：[chapter-12-vhost-optimization/](../chapter-12-vhost-optimization/)
- 下一章：[chapter-14-ovs-dpdk-acceleration/](../chapter-14-ovs-dpdk-acceleration/)
- 对照：[Ch5 Pipeline](../chapter-05-packet-forwarding/) · [Ch10–12 虚拟化](../chapter-10-x86-io-virtualization/) · [02-Advanced-Book](../../02-Advanced-Book/)
