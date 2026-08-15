## 12.6 使用线程提高并行性

> ↔ [Hennessy §5.1 TLP](../../../18-computer-architecture/chapter-05-thread-level-parallelism/notes/section-5.1-引言与多处理器挑战.md)


> **Ch12 §12.6** · [章导读](../README.md) · 上节 [§12.5 ←](./section-12.5-信号量与预线程化.md) · 下节 [§12.7 →](./section-12.7-其他并发问题.md)

---

- **并行 (parallelism)** — 多核上 **同时** 算；**并发 (concurrency)** — 逻辑上同时进展（可单核切换）
- **计算密集** 任务可拆给多线程；注意 **Amdahl** 串行部分上限（→ [Ch 1](../../chapter-01-tour-of-computer-systems/) 1.9、[Ch 5](../../chapter-05-optimizing-performance/)）
- **绑核 (`pthread_setaffinity_np`)** — 减迁移、稳 cache；HFT 标配

---

### 常见陷阱
1. **并发（concurrency）不等于并行（parallelism）** — 单核可并发（时间片切换），但不并行；多核才能并行
2. **Amdahl 定律限制加速比上限** — 串行部分占 10%，即使无限核也只能加速 10 倍
3. **绑核减少迁移但可能降低负载均衡** — HFT 优先确定性，绑核是标配；但多任务不均时某些核空闲

### 自测题

<details>
<summary>Q1: 并发和并行的区别？单核 CPU 能并行吗？</summary>

并发：逻辑上同时进展（时间片切换，单核可并发）。并行：物理上同时执行（需要多核）。单核 CPU 不能并行，只能并发。

</details>

<details>
<summary>Q2: Amdahl 定律是什么？对 HFT 有什么启示？</summary>

加速比上限 = 1 / (串行比例 + 并行比例/N)。即使无限核，加速比受串行部分限制。启示：优化热路径的串行部分（如锁、I/O）比增加线程数更有效。

</details>

<details>
<summary>Q3: HFT 为什么要绑核（CPU affinity）？有什么副作用？</summary>

绑核减少线程迁移（避免 cache/TLB 冷失效），降低延迟抖动。副作用：负载不均时某些核空闲；独占一个核可能浪费（非热路径任务无核可用）。HFT 通常给 tick 线程独占一个核。

</details>

<details>
<summary>Q4: 计算密集任务和 I/O 密集任务分别适合什么并发模型？</summary>

计算密集：多线程/多进程，线程数 = 核心数，绑核。I/O 密集：I/O 多路复用（epoll），单线程管理多连接，减少线程切换。HFT 网关：混合模型 — tick 线程绑核跑计算，I/O 线程用 epoll 管连接。

</details>

---

← [§12.5 ←](./section-12.5-信号量与预线程化.md) · [本章导读](../README.md) · [§12.7 →](./section-12.7-其他并发问题.md)
