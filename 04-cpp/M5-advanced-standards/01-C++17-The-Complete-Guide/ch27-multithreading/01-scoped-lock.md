# std::scoped_lock

## 多锁的问题

```cpp
std::mutex m1, m2;

// C++11 做法：std::lock + adopt_lock
{
    std::lock(m1, m2);  // 原子地锁两个 mutex（避免死锁）
    std::lock_guard<std::mutex> lk1(m1, std::adopt_lock);
    std::lock_guard<std::mutex> lk2(m2, std::adopt_lock);
    // 使用受保护的数据
}
// 两步：先 lock 再 guard，容易漏写 adopt_lock

// 错误做法：分别锁 → 死锁风险
{
    std::lock_guard<std::mutex> lk1(m1);  // 线程 A 先锁 m1
    std::lock_guard<std::mutex> lk2(m2);  // 线程 B 先锁 m2 → 死锁！
}
```

## scoped_lock：一步到位

```cpp
// C++17：可变参数 RAII，一步锁多个
{
    std::scoped_lock lk(m1, m2);  // 原子锁 m1 和 m2，避免死锁
    // 使用受保护的数据
}
// 析构时解锁，顺序与锁相反

// 单 mutex：等价于 lock_guard
{
    std::scoped_lock lk(m1);
    // ...
}

// 零个 mutex：合法但无用
std::scoped_lock<> lk;  // 不锁任何东西
```

## 死锁避免原理

`scoped_lock` 内部使用 `std::lock(m1, m2, ...)` 的死锁避免算法：
- 尝试用 `try_lock` 逐个锁
- 如果某个 `try_lock` 失败，释放已锁的，重新尝试
- 保证不会死锁（类似银行家算法的简化版）

## 与其他锁守护对比

| 守护 | 可锁数量 | 可延迟锁定 | 可解锁 | C++ 版本 |
|------|---------|-----------|--------|---------|
| `lock_guard` | 1 | 否 | 否 | C++11 |
| `unique_lock` | 1 | 是 | 是 | C++11 |
| `scoped_lock` | N | 否 | 否 | C++17 |

## CTAD 简化

```cpp
// C++17 CTAD：不需要写模板参数
std::scoped_lock lk(m1, m2);  // 自动推导 scoped_lock<M1, M2>

// C++17 前（如果 scoped_lock 存在的话）：
std::scoped_lock<std::mutex, std::mutex> lk(m1, m2);
```

## 自测题

1. `scoped_lock` 相比 `std::lock` + `lock_guard` 有什么优势？
2. `scoped_lock` 如何避免死锁？
3. 单 mutex 时 `scoped_lock` 和 `lock_guard` 等价吗？
4. `scoped_lock` 能延迟锁定吗？能手动解锁吗？
5. C++17 CTAD 如何简化 `scoped_lock` 的写法？
