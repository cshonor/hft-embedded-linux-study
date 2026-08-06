# 5.3 std::atomic 操作

> 第 5 章 · 上一节：[5.2 六种内存序](02-memory-orders.md) · 下一节：[5.4 CAS](04-cas.md)

## 这节讲什么

`std::atomic` 的基本操作：store/load/fetch_add/exchange，以及 `is_lock_free` 检查。

---

## 核心操作

```cpp
std::atomic<int> x{0};

x.store(1, std::memory_order_release);        // 写
int v = x.load(std::memory_order_acquire);    // 读
x.fetch_add(1, std::memory_order_relaxed);    // RMW（读-改-写）
bool ok = x.compare_exchange_strong(expected, desired);  // CAS

x.exchange(5);    // 原子交换
x.is_lock_free(); // 是否真正无锁（可能用内部 mutex）
```

### is_lock_free

```cpp
std::atomic<BigStruct> a;
if (a.is_lock_free()) {
    // 真正无锁（CPU 原子指令）
} else {
    // 内部用 mutex（有锁）——性能差
}
```

大于机器字长的类型（如 128 位）可能不是 lock-free——编译器用内部 mutex 实现。

---

## 新手要点

- **`atomic` 不一定无锁**：`is_lock_free()` 检查是否真正用 CPU 原子指令。大类型可能退化为 mutex。
- **常用操作**：`load`/`store` 读写、`fetch_add`/`fetch_sub` 原子加减、`exchange` 原子交换、`compare_exchange` CAS。

---

## HFT 关联

- **行情计数器**：`std::atomic<uint64_t>` 的 `fetch_add(relaxed)` 用于吞吐量统计，无同步开销。
- **检查 lock_free**：HFT 热路径的原子变量必须 `is_lock_free()`，否则退化为 mutex 有调度抖动。

---

## 自测题

1. `is_lock_free()` 检查什么？为什么大类型可能不是 lock-free？
2. `atomic` 的常用操作有哪些？
3. `fetch_add` 的内存序默认是什么？什么时候用 `relaxed`？

---

## 参考与延伸

- 下一节：[5.4 CAS](04-cas.md)
- 回到：[第 5 章](README.md)
