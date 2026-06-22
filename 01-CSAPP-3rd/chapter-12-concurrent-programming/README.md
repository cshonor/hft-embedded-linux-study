# Ch 12 并发编程 · Concurrent Programming

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part III · 全书终章）

> 本章定位：**三种并发服务器模型**（进程 / I/O 多路复用 / 线程）+ **信号量同步** + **线程安全与死锁**。HFT 里 **行情解析、订单路由、风控** 几乎都在多线程或 reactor 上跑；本章是理解 **锁、竞争、伪共享** 的 POSIX 地基，无锁与 memory order 见 [11-HFT](../../11-HFT-Low-Latency-Practice/)。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 12.1–12.2 进程与 I/O 多路复用 | [notes/section-12.1-12.2-进程与IO多路复用.md](./notes/section-12.1-12.2-进程与IO多路复用.md) |
| 12.3–12.4 线程与共享变量 | [notes/section-12.3-12.4-线程与共享变量.md](./notes/section-12.3-12.4-线程与共享变量.md) |
| 12.5 信号量与预线程化服务器 | [notes/section-12.5-信号量与预线程化.md](./notes/section-12.5-信号量与预线程化.md) |
| 12.6–12.8 并行与其他并发问题 | [notes/section-12.6-12.8-并行与其他并发问题.md](./notes/section-12.6-12.8-并行与其他并发问题.md) |

---

## 大白话 · 本章一条线

> **同一台机器上同时干多件事：要么多进程、要么单线程事件循环、要么多线程；共享数据必须同步。**

```
accept 新连接
  ├─ fork 子进程（12.1）
  ├─ select/epoll 事件驱动（12.2）
  └─ 线程池 / 预线程化（12.3 / 12.5）

共享计数器？ → P/V 或 mutex → 否则 race
```

**HFT 三件事：**

1. **I/O 密集** — reactor（`epoll`）+ 非阻塞 fd；少 `fork`（→ [08-UNP](../../08-UNP-Vol1/)）
2. **共享状态** — 订单簿、持仓：锁粒度、无锁队列、**false sharing**（→ [Ch 6](../chapter-06-memory-hierarchy/)）
3. **正确性 > 吞吐** — 死锁、可重入、`errno` 线程局部；热路径用 **SPSC 无共享** 设计

---

## 本章 Checklist

- [ ] 对比三种并发服务器：进程 / `select` / 线程池
- [ ] 会写 `pthread_create` / `pthread_join` / detach
- [ ] 画出进度图，解释 **race** 与 **互斥**
- [ ] 会用信号量实现 **生产者-消费者**、**预线程化 Echo**
- [ ] 区分 **线程安全** vs **可重入**；知道 `strtok` 类陷阱
- [ ] 能举 **死锁** 四条件；说出 HFT 减锁策略

---

## HFT 精读捷径

```
12.2 I/O 多路复用 — 与 Ch11 + UNP epoll 连读
12.3 线程 API — 必读
12.5 信号量 + 预线程化 — 理解线程池语义
12.7 线程安全/race/死锁 — 必读
12.6 CPU 并行 — 绑核、少超订；与 Ch5 Amdahl 呼应
12.1 fork 服务器 — 知道即可，生产少用
延伸：11-HFT 无锁 · 02-SysPerf Ch6 CPU · C++11 memory_order
```

---

## 相关章节

- 上一章：[../chapter-11-network-programming/](../chapter-11-network-programming/)
- 附录：[../appendix-A-错误处理.md](../appendix-A-错误处理.md)
- 异常与进程：[../chapter-08-exceptional-control-flow/](../chapter-08-exceptional-control-flow/)
- Cache / 伪共享：[../chapter-06-memory-hierarchy/](../chapter-06-memory-hierarchy/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
