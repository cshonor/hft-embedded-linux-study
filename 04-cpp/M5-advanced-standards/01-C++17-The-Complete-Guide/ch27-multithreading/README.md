# 第 27 章 多线程与并发库

**Multi-Threading and Concurrency**

## 本章讲什么

C++17 对并发的增量改进：`scoped_lock`、`shared_mutex`、并行算法（见第 22 章）。大部分并发特性在 C++11/14 已引入，C++17 是补充。（深入并发见 [2-Cpp-Concurrency](../../../M3-deep-principles/02-Cpp-Concurrency/)。）

## 要点

### `std::scoped_lock`（C++17）

```cpp
std::mutex m1, m2;
// C++17 之前：std::lock(m1, m2) + lock_guard(unique_lock) 两步
std::scoped_lock lk(m1, m2);   // 一步：RAII 同时锁多个，原子无死锁
```

`scoped_lock` 是 `std::lock` 的 RAII 封装，同时锁多个 mutex 且避免死锁。单 mutex 时等价于 `lock_guard`。

### `std::shared_mutex`（C++17）

```cpp
std::shared_mutex rw;
// 读：多读者并发
{
    std::shared_lock lk(rw);   // 共享读锁
    read_data();
}
// 写：独占
{
    std::unique_lock lk(rw);   // 独占写锁
    write_data();
}
```

读写锁，多读少写场景提高并发度。C++14 有 `std::shared_timed_mutex`，C++17 的 `shared_mutex` 去掉了超时能力，性能更好。

### `std::atomic::is_always_lock_free`

```cpp
static_assert(std::atomic<BigStruct>::is_always_lock_free);
// 编译期常量，保证该原子类型在所有平台上都无锁
```

C++17 的 `is_always_lock_free` 是编译期常量，用于静态断言——`is_lock_free()` 是运行期的，不能用于 static_assert。

### 并行算法（见第 22 章）

C++17 并行 STL 已在第 22 章详述，本章是 C++17 库视角的概览。

### 硬件干涉大小（C++17 提案，实际 C++20 落地）

`std::hardware_destructive_interference_size`（64 通常是 cache 行大小）——C++17 提出但实际进 C++20，用于 `alignas` 防伪共享。

## HFT 关联

- **`scoped_lock` 多资源原子锁**：订单簿跨多结构操作用 `scoped_lock(book_mutex, order_mutex)` 一步锁，避免分别锁的死锁窗口。
- **`shared_mutex` 行情快照读多写少**：行情快照多策略读、单线程写，用 `shared_lock` 并发读不互斥。
- **`is_always_lock_free` 编译期保证**：热路径原子结构 `static_assert(atomic<T>::is_always_lock_free)` 确保无内部锁。
- **热路径仍慎用 shared_mutex**：`shared_mutex` 读路径有原子计数开销，极高频率下不如无锁快照。
- **与 08 的衔接**：本章是 C++17 新增并发工具的索引，深入原理和陷阱见 02-Cpp-Concurrency。

## 自测题

1. `scoped_lock` 相比 `std::lock` + `lock_guard` 有什么优势？
2. `shared_mutex` 和 `shared_timed_mutex` 的区别？
3. `is_always_lock_free` 和 `is_lock_free()` 的区别？为什么需要编译期版本？
4. HFT 订单簿跨多结构操作如何用 `scoped_lock`？
5. 为什么 HFT 热路径仍可能不用 `shared_mutex`？

## 代码自测

### Q1: shared_mutex
```cpp
std::shared_mutex rw_mutex;

// 读操作：多个线程可同时持有共享锁
void read_data() {
    std::shared_lock lk(rw_mutex);  // 共享锁
    // 读数据...
}

// 写操作：独占锁
void write_data() {
    std::unique_lock lk(rw_mutex);  // 独占锁
    // 写数据...
}
```
> shared_mutex（读写锁）相比普通 mutex 有什么优势？什么场景适合？

<details>
<summary>答案与复习指引</summary>

**读写锁优势**：允许多个读线程并发，只有写线程独占。读多写少的场景吞吐量大幅提升。

**适用场景**：
- 配置表/查找表：读极频繁、偶尔更新 → 读读并发
- 缓存：读多写少

**不适用场景**：
- 读写频率相近 → 读写锁的额外开销（原子操作 + 状态跟踪）抵消并发收益
- 写操作多 → 写线程饥饿（读线程持续获取共享锁，写线程等不到独占锁）

**HFT 注意**：shared_mutex 比 mutex 重（内部有原子计数 + 状态机），低竞争场景反而更慢。且共享锁的 cache line 争用（所有读者同一 cache line）可能成为瓶颈。

**复习：** → [shared_mutex](./README.md)
</details>
