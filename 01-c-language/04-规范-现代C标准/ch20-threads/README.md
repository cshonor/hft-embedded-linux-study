# Ch20 · Threads（线程） ⑤🔴

> **Level 3 · 深入** · 策略：**🔴 精读**（进 DPDK 前必读）· 阅读顺序 ⑤
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

> **第 4 本书 · Ch20** · DPDK 每 lcore 一线程模型 = `_Thread_local` + 绑核，虽然生产用 pthread，
> 但 `threads.h` 是理解模型的最短路径。

## 本章讲什么

C11 `threads.h`（`thrd_create`/`join`/`detach`）、线程局部数据（`tss`/`_Thread_local`）、
互斥锁（`mtx`）、条件变量（`cnd`）、线程管理策略。

## 小节索引

| 节 | 标题 | 核心知识点 |
|----|------|------------|
| [20.1](./20.1-简单线程间控制.md) | 简单线程间控制 | `thrd_create`/`join`/`detach`；线程函数签名 |
| [20.2](./20.2-线程局部数据.md) | 线程局部数据 | `_Thread_local`（编译期）vs `tss_t`（运行期） |
| [20.3](./20.3-互斥锁.md) | 互斥锁 | `mtx_t` 类型；临界区规则；死锁避免 |
| [20.4](./20.4-条件变量.md) | 条件变量 | `cnd_wait`/`signal`/`broadcast`；虚假唤醒 |
| [20.5](./20.5-线程管理策略.md) | 线程管理策略 | pthread vs threads.h；HFT 生产用 pthread |
| [20.6](./20.6-DPDK线程模型.md) | DPDK 线程模型 | lcore 绑核模型；轮询 vs 阻塞；100% CPU 换最低延迟 |

## HFT / DPDK 关联总结

| 概念 | HFT / DPDK 应用 |
|------|-----------------|
| **`_Thread_local`** | 每 lcore 独立数据（统计计数器、本地缓存） |
| **线程绑核** | 每 lcore 绑定一个 CPU 核心（`pthread_setaffinity_np`） |
| **轮询模型** | 100% CPU 持续轮询，不睡眠，最低延迟 |
| **无锁数据结构** | `rte_ring` 连接各 lcore（不用 mutex，见 [Ch21](../ch21-atomic-access-memory-consistency/README.md)） |
| **mutex 只在初始化** | 热路径完全不用锁 |
| **条件变量不用** | 轮询代替等待 |

## 自测题

<details><summary>1. DPDK 为什么用轮询而不是阻塞/条件变量？</summary>

阻塞和条件变量有上下文切换开销——线程睡眠后被唤醒需要微秒级延迟（内核调度、cache miss）。
HFT 要求纳秒级响应，轮询模式下线程始终运行（100% CPU），数据到达后立刻处理，没有唤醒延迟。
代价是 CPU 占用率高，但对 HFT 来说延迟比 CPU 利用率重要。
</details>

<details><summary>2. <code>_Thread_local</code> 和全局变量有什么区别？为什么 HFT 用它？</summary>

全局变量所有线程共享——修改需要同步（锁或原子操作），有 contention 和 cache 伪共享问题。
`_Thread_local` 每线程独立一份——互不干扰，无需同步，无 contention。HFT 中每 lcore 的统计
计数器用 `_Thread_local`：每个线程只更新自己的副本，汇总时才读取所有线程的值（无锁读取，
因为运行时各 lcore 只写自己的）。内核的 `DEFINE_PER_CPU` 是同一概念。
</details>

<details><summary>3. 为什么 <code>cnd_wait</code> 要在 <code>while</code> 循环中？</summary>

防止虚假唤醒 (spurious wakeup)——`cnd_wait` 可能在没有 `cnd_signal` 的情况下返回
（POSIX 允许实现这样）。如果在 `if` 中，虚假唤醒后不会重新检查条件，可能处理了未准备好的数据。
`while` 循环确保每次唤醒后都重新检查条件，只有条件为真才继续执行。
</details>

<details><summary>4. HFT 热路径为什么不用 mutex？用什么替代？</summary>

mutex 有上下文切换开销（竞争时）、不确定延迟（等锁时间取决于其它线程）、cache 伪共享问题。
HFT 热路径替代方案：① 每 lcore 独立数据（`_Thread_local`）——完全不共享；
② 无锁队列（`_Atomic` + 内存序）——共享但无锁，见 [Ch21](../ch21-atomic-access-memory-consistency/README.md)；
③ 批量处理——减少队列操作频率，amortize 同步成本。
mutex 只在初始化/配置阶段使用，运行时完全不碰锁。
</details>

<details><summary>5. C11 <code>threads.h</code> 和 <code>pthread</code> 在 HFT 中怎么选？</summary>

学习/模型理解用 `threads.h`（C 标准，概念清晰）；生产环境用 `pthread`（功能全：CPU 亲和性、
实时调度、信号掩码、futex）。DPDK 实际用 pthread + `pthread_setaffinity_np` 做绑核。
`threads.h` 不支持绑核和调度策略，这是 HFT 的硬需求，所以生产只能用 pthread。
</details>
