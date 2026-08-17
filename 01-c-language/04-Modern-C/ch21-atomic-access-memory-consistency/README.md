# Ch21 · Atomic access and memory consistency（原子访问与内存一致性） ⑥🔴

> **Level 3 · 深入** · 策略：**🔴 精读**（全书压轴、DPDK rte_ring 的理论基础）· 阅读顺序 ⑥
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

> **第 4 本书 · Ch21** · 读懂本章才能读懂 DPDK `rte_ring` 无锁队列源码。

## 本章讲什么

数据竞争与 UB、**happens-before 关系**、**五种内存序**（`seq_cst` / `acquire` / `release` /
`acq_rel` / `relaxed`）、原子操作 vs 锁的对比、内存屏障。

## 小节索引

| 节 | 标题 | 核心知识点 |
|----|------|------------|
| [21.1](./21.1-数据竞争与happens-before.md) | 数据竞争与 happens-before | 数据竞争 = UB；`_Atomic` 解决方案；hb 的五种建立方式 |
| [21.2](./21.2-原子操作API.md) | 原子操作 API | load/store/exchange/CAS；RMW 操作；原子 vs 锁 |
| [21.3](./21.3-五种内存序.md) | 五种内存序 | **核心**：seq_cst/acquire/release/acq_rel/relaxed；acquire+release 配对 |
| [21.4](./21.4-DPDK-rte_ring与内存屏障.md) | DPDK rte_ring 与内存屏障 | rte_ring 源码级分析；MPMC CAS；x86 vs ARM 内存模型 |

## HFT / DPDK 关联总结

| 概念 | DPDK 应用 |
|------|----------|
| **acquire/release 配对** | rte_ring 的 head/tail 更新 |
| **`relaxed`** | 每 lcore 统计计数器 |
| **CAS 循环** | rte_ring MPMC 入队/出队 |
| **`_Alignas(64)` + `_Atomic`** | 防伪共享 + 原子更新 |
| **acq_rel** | CAS 操作（同时获取和释放） |
| **内存屏障** | `rte_smp_wmb()`/`rte_smp_rmb()`（C11 前）→ `_Atomic`（C11 后） |

### rte_ring 内存序映射

| rte_ring 操作 | C11 内存序 | 说明 |
|---------------|-----------|------|
| 生产者读自己的 head | `relaxed` | 单生产者，不需要同步 |
| 生产者读消费者的 tail | `acquire` | 必须看到消费者释放的 slot |
| 生产者写 head | `release` | 让消费者看到 slots 写入 |
| 消费者读自己的 tail | `relaxed` | 单消费者，不需要同步 |
| 消费者读生产者的 head | `acquire` | 必须看到生产者写入的 slots |
| 消费者写 tail | `release` | 让生产者看到 slot 已释放 |
| MPMC CAS | `acq_rel` | 同时获取和释放 |

## 自测题

<details><summary>1. 为什么 <code>int counter = 0; counter++;</code> 在多线程下不安全？</summary>

`counter++` 不是原子操作——它做三步：读 counter、加 1、写回 counter。两个线程同时执行时，
可能都读到旧值 0，各自加 1 后写回 1，结果丢失了一次更新。C11 标准规定有数据竞争的程序是 UB。
解决：用 `_Atomic int counter` + `atomic_fetch_add(&counter, 1)`，硬件保证读-改-写不可分割。
</details>

<details><summary>2. acquire 和 release 怎么配对建立 happens-before？</summary>

生产者用 `release` 存储（`atomic_store(&flag, 1, memory_order_release)`），消费者用 `acquire`
加载（`atomic_load(&flag, memory_order_acquire)`）。当 acquire 加载看到 release 存储的值时，
happens-before 关系建立：release 之前的所有写操作对 acquire 之后的读操作可见。
这是无锁数据结构传递数据的标准模式——rte_ring 的 head/tail 更新就是这个模式。
</details>

<details><summary>3. 为什么 rte_ring 读自己的 head/tail 用 <code>relaxed</code>，读对方的用 <code>acquire</code>？</summary>

读自己的 head/tail 不需要与其它线程同步——单生产者只有一个线程写 head，读自己的变量用 relaxed
就够了（程序顺序保证）。读对方的 head/tail 需要 acquire——必须看到对方 release 之前的所有写
（即 slots 数据），否则可能读到未初始化的数据。这是 SPSC 环的优化：自己的变量不需要同步开销。
</details>

<details><summary>4. <code>memory_order_relaxed</code> 什么时候安全使用？</summary>

当你只关心操作本身的原子性，不关心与其它操作的顺序时。典型场景：① 统计计数器
（`rx_pkts++`，只关心数字正确，不关心与数据处理的顺序）；② 引用计数递增（只加不减，
不涉及释放）。不安全场景：发布数据（需要 release）、获取数据（需要 acquire）、
CAS 更新（需要 acq_rel）。
</details>

<details><summary>5. x86 上测试正确的无锁代码为什么在 ARM 上可能出错？</summary>

x86 是强内存模型（TSO），大部分重排被硬件禁止——比如 Store-Store 不重排，所以
`data=42; ready=1;` 在 x86 上天然保证 ready=1 时 data 已写。ARM 是弱内存模型，
Store-Store 可能被重排——`data=42; ready=1;` 在 ARM 上 ready 可能先于 data 可见。
必须用 C11 原子操作（`atomic_store(&ready, 1, memory_order_release)`）保证顺序，
不能依赖平台内存模型。
</details>

<details><summary>6. DPDK rte_ring 为什么不用 mutex？</summary>

mutex 有上下文切换开销（竞争时）、不确定延迟（等锁时间取决于其它线程）、不适合 HFT 热路径。
rte_ring 用 `_Atomic` + acquire/release 内存序实现无锁队列：生产者用 release 发布 head，
消费者用 acquire 获取 head——数据通过 happens-before 关系传递，不需要锁。延迟在纳秒级
（CAS 指令），比 mutex 快 1000 倍。
</details>
