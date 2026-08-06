# 6.5 读写锁的应用

> 第 6 章 · 上一节：[6.4 设计准则](04-design-guidelines.md) · 下一章：[第 7 章 设无锁数据结构](../ch07-lock-free-containers/01-lock-free-vs-locked.md)

## 这节讲什么

读多写少的场景，用**读写锁**（`std::shared_mutex`）让多个读操作并行、写操作独占。本节讲读写锁的正确用法、写者饥饿问题、以及升级锁（read→write）的陷阱。

---

## 核心规则（代码+表格）

### 读写锁基础

```cpp
#include <shared_mutex>

template <typename K, typename V>
class rwlock_map {
    std::unordered_map<K, V> data;
    mutable std::shared_mutex m;  // C++17 读写锁
public:
    // 读：多个线程可同时持有 shared_lock
    V get(const K& k) const {
        std::shared_lock<std::shared_mutex> lk(m);  // 读锁
        auto it = data.find(k);
        return it != data.end() ? it->second : V{};
    }
    // 写：独占
    void set(const K& k, const V& v) {
        std::unique_lock<std::shared_mutex> lk(m);  // 写锁
        data[k] = v;
    }
    // 读锁不能升级为写锁！需先释放再获取写锁
};
```

### `shared_mutex` 接口

| 操作 | 锁类型 | 说明 |
|------|--------|------|
| `lock()` / `unique_lock` | 写锁（独占） | 阻塞所有其他读写 |
| `lock_shared()` / `shared_lock` | 读锁（共享） | 与其他读锁并行，阻塞写 |
| `try_lock()` | 尝试写锁 | 非阻塞 |
| `try_lock_shared()` | 尝试读锁 | 非阻塞 |

### 写者饥饿问题

标准 `shared_mutex` 不保证公平性。如果读操作持续不断，写操作可能**永远等不到锁**：

```
读者1 持锁 → 读者2 来了也拿到 → 读者3 → ... → 写者一直等
```

解决方式：
- 部分实现有"写者优先"模式（标准库不保证）。
- 限制读并发数，或用信号量手动控制。
- 写少时影响不大；写频繁时读写锁可能比普通 mutex 更差。

### 读锁升级陷阱

```cpp
// 危险：读锁不能升级为写锁
V get_or_default(const K& k, const V& def) {
    std::shared_lock<std::shared_mutex> lk(m);  // 读锁
    auto it = data.find(k);
    if (it != data.end()) return it->second;
    // 想在这里升级为写锁来 set(k, def) —— 不行！
    // 两个线程同时升级 → 互相等对方释放读锁 → 死锁
}
// 正确：先释放读锁，再获取写锁（但中间可能被其他写者插入）
V get_or_default_safe(const K& k, const V& def) {
    {
        std::shared_lock<std::shared_mutex> lk(m);
        auto it = data.find(k);
        if (it != data.end()) return it->second;
    }  // 读锁释放
    std::unique_lock<std::shared_mutex> lk(m);  // 重新获取写锁
    // 再次检查（double-checked）
    auto it = data.find(k);
    if (it != data.end()) return it->second;
    data[k] = def;
    return def;
}
```

---

## 新手要点（和 C 的区别）

- **C 里的读写锁是 `pthread_rwlock_t`**：POSIX 提供，语义和 `std::shared_mutex` 类似。C++17 的 `shared_mutex` 是标准库版本，跨平台。
- **C 程序员常犯"读锁升级"的错**：`pthread_rwlock` 也没有安全的升级机制。两个线程同时 `rwlock_rdlock` → 想升级到 `rwlock_wrlock` → 互等 → 死锁。正确做法是释放读锁再获取写锁（double-checked）。
- **`shared_lock` 是 C++14 引入的 RAII**：C 里用 `pthread_rwlock_rdlock`/`pthread_rwlock_unlock` 手动配对，容易忘 unlock。C++ 的 `shared_lock` 析构自动释放。
- **读写锁不总是更快**：C 程序员可能觉得"读多写少就该用读写锁"。但 `shared_mutex` 比 `mutex` 更重（要维护读者计数），低竞争或写频繁时可能更慢。要实测。

---

## HFT 关联

- **行情快照表读多写少**：全市场快照表，写（行情更新）频率远低于读（策略查询）。`shared_mutex` 在这里非常合适——多个策略线程可并行读快照。
- **写者饥饿的 HFT 风险**：如果行情更新（写）被读者饿死，快照会过期，策略基于旧数据决策——灾难。HFT 可能需要写者优先的定制锁，或限制读者并发数。
- **锁升级在 HFT 中要避免**：HFT 系统高并发下，锁升级导致的死锁会触发 `std::terminate`（默认 `std::system_error`）。一律用 double-checked 模式。
- **`shared_mutex` 的性能特性**：x86 上 `shared_mutex` 的读路径有原子 `fetch_add`（读者计数），比 `mutex` 的 `lock()` 重。在读极频繁、写极少的 HFT 场景值得，但要 benchmark。

---

## 自测题

1. `shared_mutex` 的读锁和写锁分别用什么 RAII 守卫？
2. 什么是写者饥饿？什么场景下会发生？
3. 为什么读锁不能直接升级为写锁？正确做法是什么？
4. 读写锁在什么情况下比普通 `mutex` 更慢？
5. `get_or_default` 的 double-checked 模式中，第二次检查为什么必要？

---

## 参考与延伸

- 下一章：[7.1 无锁 vs 有锁](../ch07-lock-free-containers/01-lock-free-vs-locked.md)
- 上一节：[6.4 设计准则](04-design-guidelines.md)
- 回到：[第 6 章](README.md)
