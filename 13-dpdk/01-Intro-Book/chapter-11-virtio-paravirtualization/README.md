# Ch 11 半虚拟化 Virtio · Virtio Paravirtualization

> **01-Intro-Book** · 《深入浅出 DPDK》第十一章 · **🟡 虚拟化篇**  
> Virtqueue、特性协商、DPDK 固定环表与 Indirect 描述符

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. Virtio 规范与场景 | [notes/section-2-Virtio规范与使用场景.md](./notes/section-2-Virtio规范与使用场景.md) |
| 3. 虚拟队列 Virtqueue | [notes/section-3-虚拟队列机制.md](./notes/section-3-虚拟队列机制.md) |
| 4. 内核与 DPDK 驱动架构 | [notes/section-4-内核与DPDK驱动架构.md](./notes/section-4-内核与DPDK驱动架构.md) |
| 5. DPDK 深度优化 | [notes/section-5-DPDK深度优化.md](./notes/section-5-DPDK深度优化.md) |
| 6. 小结与索引 | [notes/section-6-小结与索引.md](./notes/section-6-小结与索引.md) |

---

## HFT 视角：virtio 的账算在哪

virtio 的优化史（特性协商、indirect 描述符、mergeable buffer）本质上是在** amortize 一类固定成本**：

```
guest 发一个包 → virtqueue 放描述符 → kick（写 MMIO）→ VM-exit
→ host 后端处理 → 中断注回 guest（又一次 world switch）
```

**每次 kick/notify 都是一次 VM-exit，µs 级且抖动不可控**——
这对吞吐型业务（用批量化摊薄）是可接受的，对 HFT 的 p999 是致命的：
你优化掉的每一级缓存 miss 是几十 ns，而一次 VM-exit 是几千 ns。

所以结论很干脆：

- **生产行情/订单路径**：不接受 virtio。云厂商的"增强网络"大多是 virtio 变体，先问清
- **必须 VM**：走 [Ch10](../chapter-10-x86-io-virtualization/) 的 SR-IOV 透传，跳过整个 virtio 层
- **开发/回测环境**：virtio 随便用——功能正确性测试不吃尾延迟
- 本章真正值得带走的是 **virtqueue 的环设计**：它和 [Ch6 DMA 描述符环](../../chapter-06-pcie-packet-io/) 同源，
  是理解 [Ch12 vhost](../chapter-12-vhost-optimization/) 如何把后端搬进用户态的钥匙

---

## 相关

- 上一章：[chapter-10-x86-io-virtualization/](../chapter-10-x86-io-virtualization/)
- 下一章：[chapter-12-vhost-optimization/](../chapter-12-vhost-optimization/)
- 对照：[Ch10 透传](../chapter-10-x86-io-virtualization/notes/section-2-X86虚拟化概述.md) · [Ch6 mbuf](../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md)
