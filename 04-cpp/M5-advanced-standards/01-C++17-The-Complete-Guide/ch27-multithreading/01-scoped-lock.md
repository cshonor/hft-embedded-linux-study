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

<details>
<summary>参考答案</summary>

1. 它把「**一次死锁安全的加锁 + RAII 自动解锁**」合成一步。原来要写 `std::lock(m1, m2);` 再配两个 `std::lock_guard<std::mutex> g1(m1, std::adopt_lock), g2(m2, std::adopt_lock);`，既啰嗦又容易漏 `adopt_lock`（漏了会重复加锁导致未定义行为/死锁）。
`scoped_lock` 还接受**任意数量**的 mutex（包括一个，也包括零个），并且是异常安全的：构造成功即全部持有，析构时全部释放。
2. 它在构造时调用 `std::lock(m...)`（或其等价的死锁避免算法）。该算法不是「按固定顺序 lock」，而是用 `try_lock` 逐个尝试：一旦某个 `try_lock` 失败，就把已拿到的锁全部释放，退避后重新尝试，直到同时拿到全部锁。
因为它从不「持有一个锁去阻塞等待另一个锁」，所以不可能形成循环等待——标准保证不会死锁（不保证公平、不保证特定获取顺序）。
3. 是的，语义等价：两者都在构造时 `lock()`、析构时 `unlock()`，都是不可拷贝、不可延迟、不可手动解锁的纯 RAII 守护。
差别只在写法与可读性（以及 C++17 起 `scoped_lock` 支持 CTAD）。单 mutex 场景用哪个都行，`lock_guard` 也依然是合法且清晰的选择。
4. **不能延迟锁定**：`scoped_lock` 没有 `defer_lock` 构造，构造即加锁（这点与 `unique_lock` 不同）。
**不能手动解锁**：它没有 `unlock()` 成员，只能等析构时自动释放（同样不同于 `unique_lock`）。
唯一的例外是 `std::adopt_lock` 构造：`std::scoped_lock lk(std::adopt_lock, m1, m2);` 表示「这几个锁已经由调用方持有了，我只是接管并在析构时释放」——但这仍然不是延迟加锁。
5. C++17 的推导指引可以从构造函数实参直接推出模板参数列表，于是不用手写那一长串 `std::scoped_lock<std::mutex, std::shared_mutex, ...>`：
```cpp
std::mutex m1, m2;
std::scoped_lock lk(m1, m2);   // 自动推导为 scoped_lock<std::mutex, std::mutex>
```
mutex 类型一多、一改，省下的维护成本很明显；单锁时 `std::scoped_lock lk(m1);` 也成立。

</details>
