# 11.3 死锁的检测

> 第 11 章 · 上一节：[11.2 数据竞争的检测](02-data-race.md) · 下一节：[11.4 并发测试策略](04-testing.md)

## 这节讲什么

死锁（deadlock）= 两个或更多线程互相等待对方的锁，永远阻塞。本节讲死锁的四个必要条件、TSan 的锁序检测、以及如何用 `std::scoped_lock` 避免死锁。

---

## 核心规则（代码+表格）

### 死锁的四个必要条件

| 条件 | 说明 | 打破方式 |
|------|------|---------|
| 互斥 | 锁被独占 | 无法打破（锁的本质） |
| 持有并等待 | 持有锁 A，等锁 B | 一次性获取所有锁（scoped_lock） |
| 不可剥夺 | 不能强行抢锁 | `try_lock` + 超时（不推荐） |
| 循环等待 | A 等 B，B 等 A | 固定锁序 |

### 经典死锁示例

```cpp
std::mutex m1, m2;

// 线程1：先锁 m1 再锁 m2
void t1() {
    std::lock_guard<std::mutex> a(m1);
    std::lock_guard<std::mutex> b(m2);
    // ...
}

// 线程2：先锁 m2 再锁 m1
void t2() {
    std::lock_guard<std::mutex> b(m2);
    std::lock_guard<std::mutex> a(m1);
    // ...
}
// t1 持有 m1 等 m2，t2 持有 m2 等 m1 → 死锁
```

### 解法1：`std::scoped_lock`（C++17）

```cpp
// scoped_lock 用死锁 avoidance 算法一次性获取所有锁
void safe_t1() {
    std::scoped_lock lk(m1, m2);  // 原子地获取两把锁，无死锁
    // ...
}
void safe_t2() {
    std::scoped_lock lk(m2, m1);  // 顺序不同也安全
    // ...
}
// scoped_lock 内部用 std::lock(m1, m2) 的 try-and-back-off 算法
```

### 解法2：固定锁序

```cpp
// 规定：永远先锁地址较小的 mutex
void safe_t1() {
    auto& first = &m1 < &m2 ? m1 : m2;
    auto& second = &m1 < &m2 ? m2 : m1;
    std::lock_guard<std::mutex> a(first);
    std::lock_guard<std::mutex> b(second);
}
// 两个线程都按相同顺序加锁 → 不会循环等待
```

### TSan 的锁序检测

```bash
g++ -fsanitize=thread -g -O1 deadlock.cpp -o deadlock
./deadlock
# TSan 检测到潜在死锁（锁序反转）：
# WARNING: ThreadSanitizer: lock-order-inversion (potential deadlock)
# Cycle: lock m1 → lock m2 (in T1)
#        lock m2 → lock m1 (in T2)
# TSan 不需要实际死锁发生，只要检测到"可能死锁的锁序"就报警
```

### 活锁（livelock）

```cpp
// 活锁：线程不阻塞但也不推进
void t1() {
    while (true) {
        std::unique_lock<std::mutex> a(m1, std::try_to_lock);
        if (!a.owns_lock()) continue;
        std::unique_lock<std::mutex> b(m2, std::try_to_lock);
        if (!b.owns_lock()) continue;  // 放弃 a，重试
        // work
        return;
    }
}
void t2() {
    while (true) {
        std::unique_lock<std::mutex> b(m2, std::try_to_lock);
        if (!b.owns_lock()) continue;
        std::unique_lock<std::mutex> a(m1, std::try_to_lock);
        if (!a.owns_lock()) continue;
        // work
        return;
    }
}
// 两个线程同时拿到第一把锁、同时尝试第二把锁失败、同时放弃、同时重试...
// → 永远循环（不阻塞，但也不完成）
```

---

## 新手要点（和 C 的区别）

- **C 的 `pthread_mutex` 没有 `scoped_lock`**：C 程序员要手动用 `pthread_mutex_lock`/`unlock` 或自己写固定锁序。C++17 的 `scoped_lock` 一次获取多把锁且无死锁——这是 C++ 的巨大优势。
- **"四个必要条件"是经典理论**：C 程序员如果学过操作系统课，应该熟悉 Coffman 四条件。但实际编程中，最容易违反的是"循环等待"——C 程序员可能不注意锁序。
- **TSan 的锁序检测是预防性的**：C 程序员可能觉得"没死锁就不用管"——但 TSan 能在死锁发生前检测到"潜在锁序反转"。这是预防胜于治疗。
- **活锁比死锁更难发现**：C 程序员可能只关注死锁——但活锁（不阻塞但无限循环）同样致命，且更难调试（CPU 100% 但无进展）。`try_lock` 回退策略容易导致活锁。

---

## HFT 关联

- **HFT 系统死锁 = 系统瘫痪**：HFT 系统如果死锁，无法下单也无法撤单——金融风险极高。HFT 对死锁是零容忍。
- **`scoped_lock` 是 HFT 的首选**：HFT 系统中如果必须持有多把锁，一律用 `scoped_lock`——C++17 的死锁 avoidance 算法比手动锁序更可靠。
- **避免多锁**：HFT 设计的最佳实践是"每线程最多持有一把锁"——通过 SPSC 队列、`thread_local` 等消除多锁需求。有多锁就有死锁风险。
- **死锁检测在 HFT 中的挑战**：HFT 生产环境不能加 TSan（性能），但 CI 要加。此外，HFT 系统的死锁可能只在特定负载下触发——压力测试 + TSan 是必须的。

---

## 自测题

1. 死锁的四个必要条件是什么？哪个最容易打破？
2. `std::scoped_lock` 如何避免死锁？它的内部算法是什么？
3. 固定锁序如何避免死锁？有什么缺点？
4. TSan 如何检测"潜在死锁"？需要实际死锁发生吗？
5. 活锁和死锁有什么区别？为什么活锁更难发现？

---

## 参考与延伸

- 下一节：[11.4 并发测试策略](04-testing.md)
- 上一节：[11.2 数据竞争的检测](02-data-race.md)
- 回到：[第 11 章](README.md)
