## 12.7 其他并发问题

> ↔ [Hennessy §5.6 一致性模型](../../../19-computer-architecture/chapter-05-thread-level-parallelism/notes/section-5.6-内存一致性模型.md)


> **Ch12 §12.7** · [章导读](../README.md) · 上节 [§12.6 ←](./section-12.6-使用线程提高并行性.md) · 下节 [§12.8 →](./section-12.8-小结.md)

---

#### 12.7.1 线程安全 (Thread Safety)

- 函数可被多线程 **并发调用** 且结果正确
- 四类：不共享 / 只读共享 / 需同步的共享 / 不可重入的库状态

#### 12.7.2 可重入性 (Reentrancy)

- 可被 **中断或重入** 再次调用仍正确
- 线程安全 ⊃ 可重入（在单线程重入意义下）；**可重入 ⇒ 线程安全**（常见表述）

#### 12.7.3 在线程化程序中使用库函数

| 陷阱 | 例子 |
|------|------|
| 返回静态缓冲区 | `ctime`, `gethostbyname`（旧） |
| 隐式全局状态 | `strtok`, `rand` |
| 替代 | `strtok_r`, `rand_r`, `localtime_r`, `getaddrinfo` |

- **`errno`** — 线程局部存储（TLS）

#### 12.7.4 竞争 (Races)

- 结果依赖 **不可控交错** — 如 `counter++` 非原子（load-add-store 三步）
- 修复：**互斥**、**原子操作**、**不共享**（最快）

#### 12.7.5 死锁 (Deadlocks)

四条件（Coffman）：

1. 互斥
2. 持有并等待
3. 不可抢占
4. 循环等待

**避免：** 锁顺序一致、超时、`trylock`、无锁结构、减锁粒度

**HFT 实践：**

```
热路径：SPSC 队列，无锁或单生产者单消费者
温路径：细粒度 mutex / shared_mutex
冷路径：日志、配置 — 可粗锁
绝不：持锁跨 I/O、嵌套锁顺序不一致
```

→ C++ `std::atomic` / `memory_order` · [18-HFT](../../../18-hft-engineering/)

---

### 常见陷阱
1. **线程安全不等于可重入** — 可重入更严格（不依赖任何外部状态），可重入一定线程安全，反之不然
2. **strtok/ctime 等返回静态缓冲区的函数不是线程安全的** — 用 strtok_r/localtime_r 等可重入版本替代
3. **死锁四条件缺一不可** — 打破任一条件即可避免：锁顺序一致（打破循环等待）、trylock（打破不可抢占）

### 自测题

<details>
<summary>Q1: 线程安全和可重入的区别？举例说明。</summary>

线程安全：多线程并发调用结果正确（可用锁实现）。可重入：被中断后重入仍正确（不依赖外部状态，不用锁）。可重入一定线程安全，线程安全不一定可重入。例：用 mutex 保护的函数线程安全但不可重入（中断时可能死锁）。

</details>

<details>
<summary>Q2: 哪些标准库函数不是线程安全的？如何替代？</summary>

返回静态缓冲区：ctime→localtime_r，gethostbyname→getaddrinfo。隐式全局状态：strtok→strtok_r，rand→rand_r。errno 是例外，用线程局部存储（TLS）实现，各线程独立。

</details>

<details>
<summary>Q3: 死锁的四个必要条件是什么？如何打破？</summary>

1) 互斥；2) 持有并等待；3) 不可抢占；4) 循环等待。打破任一即可：锁顺序一致（打破循环等待）、trylock+超时（打破不可抢占）、一次获取所有锁（打破持有并等待）。

</details>

<details>
<summary>Q4: HFT 在不同路径上如何选择同步策略？</summary>

热路径（tick）：SPSC 无锁队列，零等待零锁。温路径（风控）：细粒度 mutex/shared_mutex，短临界区。冷路径（日志/配置）：粗锁可接受。绝不：持锁跨 I/O、嵌套锁顺序不一致、热路径用 mutex。

</details>

---

← [§12.6 ←](./section-12.6-使用线程提高并行性.md) · [本章导读](../README.md) · [§12.8 →](./section-12.8-小结.md)
