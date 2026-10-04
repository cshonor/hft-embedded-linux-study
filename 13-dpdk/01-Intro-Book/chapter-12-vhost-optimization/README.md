# Ch 12 vhost 优化方案 · vhost Acceleration

> **01-Intro-Book** · 《深入浅出 DPDK》第十二章 · **🟡 虚拟化篇**  
> vhost-net → vhost-user、内存映射、vhost lib / PMD、vhost-switch

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. vhost 演进与原理 | [notes/section-2-vhost演进与原理.md](./notes/section-2-vhost演进与原理.md) |
| 3. DPDK vhost-user 设计 | [notes/section-3-DPDK用户态vhost设计.md](./notes/section-3-DPDK用户态vhost设计.md) |
| 4. vhost lib 与 vhost PMD | [notes/section-4-vhost编程与封装.md](./notes/section-4-vhost编程与封装.md) |
| 5. vhost-switch 与实战 | [notes/section-5-vhost-switch与实战.md](./notes/section-5-vhost-switch与实战.md) |
| 6. 小结与索引 | [notes/section-6-小结与索引.md](./notes/section-6-小结与索引.md) |

---

## HFT 视角：vhost-user 的价值在"同一台机器内部"

vhost 的演进（内核 vhost-net → 用户态 vhost-user）做的事，
和 DPDK 对用户态网络做的事是**同一件**：把数据面从内核搬出来，
用轮询消灭中断、用共享大页消灭拷贝——virtio 的 VM-exit 问题在**后端**被解掉了。

对 HFT 的直接相关性不强（主路径上没有 VM），但两个间接场景：

| 场景 | 说明 |
|------|------|
| **同机多实例隔离**：一台物理机上跑 N 个策略容器/VM，行情需要扇出给每个实例 | vhost-user（经 OVS-DPDK 或 vhost-switch）是共享大页零拷贝路径，比经内核 bridge 低一个量级 |
| **理解 OVS-DPDK 生态** | [Ch14](../chapter-14-ovs-dpdk-acceleration/) 的加速方案就架在 vhost-user 上；券商侧的虚拟化网关大概率是这套栈 |

**可带走的设计语言**：vhost-user 的 unix socket 控制面（传 fd、协商特性）+
共享内存数据面，是"**控制面与数据面分离**"的教科书案例——
你自己设计进程间行情分发时，同一套模式（socket 协商 + 大页 ring）直接可用。

---

## 相关

- 上一章：[chapter-11-virtio-paravirtualization/](../chapter-11-virtio-paravirtualization/)
- 下一章：[chapter-13-dpdk-nfv/](../chapter-13-dpdk-nfv/)
- 对照：[Ch8 VMDQ](../chapter-08-flow-classification-multiqueue/) · [Ch2 大页](../chapter-02-cache-and-memory/notes/section-5-大页Hugepages.md) · [Ch5 Pipeline](../chapter-05-packet-forwarding/)
