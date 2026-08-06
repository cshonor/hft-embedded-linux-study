# 7.1 无锁 vs 有锁

> 第 7 章 设无锁数据结构 · 上一章：[6.5 读写锁的应用](../ch06-lock-based-containers/05-rwlock.md) · 下一节：[7.2 无锁栈（Treiber Stack）](02-treiber-stack.md)

## 这节讲什么

无锁（lock-free）数据结构不用 mutex，靠原子操作（CAS、`fetch_add`）实现线程安全。本节讲无锁的三个层级（wait-free / lock-free / obstruction-free）、与有锁的对比、以及"什么时候该用无锁、什么时候不该用"。

---

## 核心规则（代码+表格）

### 三个进度保证层级

| 层级 | 保证 | 实现方式 | 难度 |
|------|------|----------|------|
| **wait-free** | 每个操作在**有限步**内完成 | `fetch_add`、`exchange` | 最难，罕见 |
| **lock-free** | 至少**一个**线程能在有限步推进 | CAS 自旋 | 难，常见 |
| **obstruction-free** | 无其他线程干扰时能完成 | CAS | 中 |

- **wait-free 最强**：没有饥饿，每个线程都公平推进。但很多操作（如链表插入）很难做到 wait-free。
- **lock-free 次之**：系统整体在推进，但单个线程可能持续 CAS 失败（饥饿）。
- **obstruction-free 最弱**：只保证"别人不干扰时能完成"，实际用处有限。

### 无锁 vs 有锁对比

| 维度 | 有锁（mutex） | 无锁（lock-free） |
|------|---------------|-------------------|
| 阻塞 | 持锁线程阻塞其他 | 至少一个线程总能推进 |
| 优先级反转 | 可能（低优先级持锁阻塞高优先级） | 不发生 |
| 上下文切换 | 竞争时有（内核态切换） | 无 |
| 公平性 | OS 调度决定 | 无保证（可能饥饿） |
| 实现复杂度 | 低 | 高（内存序、ABA、回收） |
| 调试难度 | 中 | 极高（不可复现 bug） |
| 适用场景 | 通用、高竞争、长临界区 | 热路径、低竞争、短临界区 |

### CAS（Compare-Exchange）是无锁的基石

```cpp
// CAS 语义：原子地"比较并交换"
bool compare_exchange_weak(T& expected, T desired);
// 如果当前值 == expected：写入 desired，返回 true
// 否则：把当前值写回 expected，返回 false
// weak 可能"伪失败"（值相等也返回 false），用在循环里

// 典型模式：CAS 自旋
std::atomic<int> counter{0};
void increment() {
    int old = counter.load();
    while (!counter.compare_exchange_weak(old, old + 1)) {
        // old 被更新为当前值，重试
    }
}
// C++11 更简单：counter.fetch_add(1); 但底层原理相同
```

### 什么时候用无锁

| 场景 | 推荐 |
|------|------|
| SPSC 队列（单生产者单消费者） | 无锁（环形缓冲，极快） |
| 计数器、序号 | `atomic`（wait-free） |
| 短临界区、低竞争 | 无锁可考虑 |
| 高竞争、长操作 | 有锁更好（CAS 重试成本 > 锁开销） |
| 复杂数据结构（树、图） | 有锁（无锁太难正确实现） |
| 需要公平性 | 有锁（无锁可能饥饿） |

---

## 新手要点（和 C 的区别）

- **C 里无锁编程极少**：C 程序员通常用 `pthread_mutex`，CAS 要靠 `__sync_bool_compare_and_swap`（GCC 内建）或汇编。C++11 的 `std::atomic` 让无锁编程标准化了。
- **"无锁不等于更快"是关键认知**：C 程序员可能觉得"无锁 = 性能好"。但在高竞争下，CAS 自旋比 mutex 更差——mutex 会让线程睡眠（不占 CPU），CAS 自旋会空耗 CPU。无锁适合低竞争热路径。
- **内存序是 C 程序员的新概念**：C 里要么用 `volatile`（错误地防优化），要么用内存屏障（`__sync_synchronize`）。C++11 的六种 `memory_order` 给了精确控制——这是第 5 章的核心内容。
- **ABA 问题是无锁特有的**：C 程序员没接触过——CAS 检查"值是否变化"，但值可能 A→B→A 回到原值，CAS 误以为没变。第 7.3 节详解。

---

## HFT 关联

- **SPSC 无锁队列是 HFT 核心数据结构**：网卡线程→策略线程的行情传递用 SPSC 环形队列，无锁、零拷贝、cache 友好。这是 HFT 性能的基石。
- **计数器用 `atomic`**：行情序号、成交量累计用 `std::atomic<uint64_t>` + `fetch_add`，wait-free 保证不阻塞。
- **不要在高竞争路径用 CAS 自旋**：HFT 系统的订单管理如果多线程高频写入同一结构，CAS 重试会比 mutex 更耗 CPU。用分段锁或减少共享。
- **无锁结构的调试地狱**：HFT 系统上线前必须用 TSan + 压力测试验证无锁结构。生产环境出现 ABA 或内存序 bug 几乎无法复现。

---

## 自测题

1. wait-free、lock-free、obstruction-free 三者有什么区别？哪个最强？
2. 无锁数据结构在高竞争下为什么可能比有锁更慢？
3. CAS 的 `compare_exchange_weak` 为什么可能"伪失败"？它应该用在哪里？
4. 什么是优先级反转？为什么无锁不会发生？
5. 什么场景适合用无锁？什么场景不适合？

---

## 参考与延伸

- 下一节：[7.2 无锁栈（Treiber Stack）](02-treiber-stack.md)
- 上一章：[6.5 读写锁的应用](../ch06-lock-based-containers/05-rwlock.md)
- 回到：[第 7 章](README.md)
