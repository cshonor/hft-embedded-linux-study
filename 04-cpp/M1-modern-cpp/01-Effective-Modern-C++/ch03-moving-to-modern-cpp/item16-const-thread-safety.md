# Item 16：让 const 成员函数线程安全

> 第 3 章 移步现代 C++ · Item 16 · 上一节：[Item 15 constexpr](item15-constexpr.md)

## 为什么要学这个（先建立直觉）

C 没有 `const` 成员函数。C 程序员只能传 `const struct Cache*` 来表达"只读"，编译器会挡住 `c->cached = 1` 这类写操作——看起来很安全。

C++ 的 `const` 成员函数承诺"不修改对象状态"。但有个后门——`mutable` 关键字：

```cpp
class Cache {
    mutable int cachedValue;
public:
    int getValue() const {              // const 函数
        if (!cachedValue) cachedValue = compute();   // 修改 mutable 成员！
        return cachedValue;
    }
};
```

**问题来了：** 如果两个线程同时调 `getValue()`，它们都在修改 `cachedValue`——这是数据竞争（data race）。标准假设 `const` 成员函数可能被并发调用，所以**只要 `const` 函数写了 `mutable` 成员，你就必须自己加锁或用 `atomic`**。

直觉反转点在于：`const` 在 C++ 里是"**逻辑** const"，不是"物理只读"。它只约束编译器（`this` 变成 `const T*`），对线程安全**零保证**。缓存、命中率计数、延迟统计——这些"不影响逻辑状态"的东西全靠 `mutable` 塞进 `const` 函数里，也正是并发风险的藏身之处。

---

## 这节讲什么

1. `const` 成员函数为什么仍可能写数据（`mutable` 后门）；
2. 数据竞争实测：两线程对裸 `mutable int` 各增 100 万次，结果远小于 200 万；
3. 两种修复：`std::atomic`（无锁）与 `std::mutex`（保护复合不变量）；
4. `mutable` 的必要性——不加 `mutable` 连锁都上不了；
5. `std::atomic` 成员的代价：类变成不可拷贝（报错原文）。

---

## 现场：const 函数里的数据竞争

```cpp
#include <iostream>
#include <thread>

class Counter {
    mutable int count = 0;                 // 裸 mutable int
public:
    void bump() const { ++count; }         // const 函数，却在写共享状态
    int  get()  const { return count; }
};

int main() {
    constexpr int kN = 1000000;
    for (int round = 1; round <= 4; ++round) {
        Counter c;
        std::thread t1([&] { for (int i = 0; i < kN; ++i) c.bump(); });
        std::thread t2([&] { for (int i = 0; i < kN; ++i) c.bump(); });
        t1.join();
        t2.join();
        std::cout << "round " << round << ": count = " << c.get()
                  << "  (expect " << 2 * kN << ")\n";
    }
    return 0;
}
```

clang 23.1.0（`-std=c++17 -Wall`，arm64 Apple）实测输出：

```
round 1: count = 1000032  (expect 2000000)
round 2: count = 1006666  (expect 2000000)
round 3: count = 1000955  (expect 2000000)
round 4: count = 1010918  (expect 2000000)
```

**关键观察：**

- 结果不是"接近 200 万"，而是**接近 100 万**——两个线程的更新几乎完全互相覆盖。arm64 上 `++count` 是"读-改-写"三条指令，两个核各自读到同一个旧值、各自写回，一半的增量凭空消失。
- **每次跑结果都不一样**（1000032 / 1006666 / 1000955 / 1010918）。这正是数据竞争的signature：不是"结果错一点"，而是**结果不确定**。
- 代码看起来完全无害：`bump()` 是 `const` 的，`get()` 也是 `const` 的，编译器一声不吭。只有 `mutable` 那个词是线索。

这类 bug 在 HFT 里的可怕之处：它不会让你的策略崩溃，它会让你的**统计数字悄悄错掉**——命中率、延迟分位数、成交量累加，全都在"看起来正常"地输出错误值。

---

## 修复方案 A：mutable std::atomic（无锁）

```cpp
class Cache {
    mutable std::atomic<int>  cachedValue{0};
    mutable std::atomic<bool> cacheValid{false};
public:
    int getValue() const {
        if (!cacheValid.load(std::memory_order_acquire)) {
            int v = compute();
            cachedValue.store(v, std::memory_order_relaxed);
            cacheValid.store(true, std::memory_order_release);
        }
        return cachedValue.load(std::memory_order_relaxed);
    }
};
```

适用：**单个变量**的读-改-写（计数器、命中标记、指针交换）。开销最小、不阻塞，HFT 热路径首选。

## 修复方案 B：mutable std::mutex（保护复合不变量）

```cpp
class Cache {
    mutable std::mutex m_;                 // 必须 mutable：const 函数里要能 lock()
    mutable int  cachedValue = 0;
    mutable bool valid = false;
public:
    int getValue() const {
        std::lock_guard<std::mutex> g(m_); // 保护 (valid, cachedValue) 这组不变量
        if (!valid) { cachedValue = compute(); valid = true; }
        return cachedValue;
    }
};
```

适用：**多个变量**要保持一致（"缓存值 + 时间戳 + 计数器"必须同时更新）。代价是阻塞与上下文切换。

**为什么锁必须 `mutable`？** 因为在 `const` 成员函数里 `this` 是 `const T*`，所有非 mutable 成员都变成 `const`；而 `mutex::lock()` 是**非 const** 成员函数——不加 `mutable` 就没法在 `const` 函数里加锁。实测报错：

```
error: no matching constructor for initialization of 'std::lock_guard<std::mutex>'
note: candidate constructor not viable: 1st argument ('const std::mutex') would lose const qualifier
```

同一个"两线程各增 100 万次"的循环，分别用裸 `mutable int` / `mutex` / `atomic` 实现，程序内部连跑三轮，实测输出：

```
run1  raw=1001447  mutex=2000000  atomic=2000000   (expect 2000000)
run2  raw=1004449  mutex=2000000  atomic=2000000   (expect 2000000)
run3  raw=1019081  mutex=2000000  atomic=2000000   (expect 2000000)
```

（整个程序再跑两遍，raw 依次是 1006051 / 1000632 / 1000000 与 1006421 / 1006333 / 1005750，mutex 与 atomic 始终精确 2000000。）

裸 `int` 每次结果都接近 100 万且**每一轮都不一样**；`mutex` 与 `atomic` 每轮都精确 200 万。差别不是"精度"而是"确定性"。

---

## 完整可运行例子

一个"逻辑只读"的行情快照：第一次取加权均价（VWAP）时才计算，之后缓存。`cachedVwap_` / `valid_` / `m_` 全都是 `mutable`：

```cpp
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

class Snapshot {
    std::vector<double> px_, sz_;              // 价格 / 数量

    mutable std::mutex m_;                     // 必须 mutable：const 函数里要能 lock()
    mutable double cachedVwap_ = 0.0;
    mutable bool   valid_      = false;
    mutable long   hits_       = 0;

    double computeVwap() const {               // 纯计算，不改逻辑状态
        double notional = 0.0, qty = 0.0;
        for (std::size_t i = 0; i < px_.size(); ++i) {
            notional += px_[i] * sz_[i];
            qty      += sz_[i];
        }
        return qty ? notional / qty : 0.0;
    }

public:
    Snapshot(std::vector<double> p, std::vector<double> s)
        : px_(std::move(p)), sz_(std::move(s)) {}

    // const 成员函数，但会写 mutable 成员 → 必须自己保证线程安全
    double vwap() const {
        std::lock_guard<std::mutex> g(m_);
        ++hits_;
        if (!valid_) { cachedVwap_ = computeVwap(); valid_ = true; }
        return cachedVwap_;
    }
    long hits() const {
        std::lock_guard<std::mutex> g(m_);
        return hits_;
    }
};

int main() {
    Snapshot snap({100.0, 101.5, 99.25}, {100, 200, 300});
    for (int i = 0; i < 3; ++i) std::cout << "vwap = " << snap.vwap() << "\n";
    std::cout << "hits = " << snap.hits() << "\n";

    // 三个线程并发调用同一个 const 成员函数
    Snapshot shared({10.0, 20.0, 30.0}, {1, 1, 1});
    double out[3];
    std::thread t0([&] { out[0] = shared.vwap(); });
    std::thread t1([&] { out[1] = shared.vwap(); });
    std::thread t2([&] { out[2] = shared.vwap(); });
    t0.join(); t1.join(); t2.join();
    std::cout << "concurrent = " << out[0] << ' ' << out[1] << ' ' << out[2]
              << ", hits = " << shared.hits() << "\n";
}
```

clang 23.1.0 实测输出：

```
vwap = 100.125
vwap = 100.125
vwap = 100.125
hits = 3
concurrent = 20 20 20, hits = 3
```

三个线程拿到的是同一个值 `20`，`hits` 恰好等于 3——懒缓存只算了一次，且计数没有丢。如果把 `m_` 去掉，同样的程序会在压力测试下给出随机的 `hits` 和撕裂的 `cachedVwap_`。

---

## 常见错误（新手踩坑）

1. **以为 `const` = 线程安全**。`const` 只保证"不修改**逻辑**状态"，编译器层面它只是把 `this` 变成 `const T*`。一旦函数通过 `mutable` 成员写共享数据，并发调用就是 data race。
2. **mutex 成员忘了加 `mutable`**，结果在 `const` 函数里根本没法 `lock()`（见上节报错原文）。同理 `mutable std::atomic<...>`——`fetch_add` 也需要非 const 对象。
3. **用 `atomic` 去保护"多个变量的一致性"**。`atomic` 只保证单个对象上的操作是原子的；两个独立的原子操作之间其他线程仍可插入，跨变量不变量会被破坏。这种场景必须用 `mutex`。
4. **在 `const` 函数里用 `double-checked locking` 手写懒缓存**。没有正确的内存序/原子类型，编译器和 CPU 的重排会让你读到"半初始化"的缓存值。用 `std::call_once` 或干脆全程加锁。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| const 方法 | 传 `const` 指针 | `const` 成员函数 | 面向对象封装 |
| mutable | 不存在 | `mutable` 关键字 | 允许 const 函数改"不影响逻辑状态"的成员 |
| 线程安全 | 手动 `pthread_mutex` | `std::mutex` / `std::atomic` | C++11 标准库 |
| 锁的声明 | 手动管理 | `mutable std::mutex` | const 函数需要能锁 |

**一句话总结：** C 程序员记住——`const` 成员函数不是"只读"的绝对保证，`mutable` 开了后门。只要 `const` 函数写了 `mutable` 成员，你就必须自己用 `atomic`（单变量）或 `mutex`（复合不变量）保证线程安全。

---

## HFT 关联

- **行情缓存**：`const` 的 `get_tick()` 如果内部更新 `mutable` 缓存，必须用 `atomic` 或 `mutex` 保护——否则多策略线程并发读会 data race，表现为缓存偶尔"算重"或读到撕裂值。
- **原子计数器**：`mutable std::atomic<uint64_t> hit_count;` 在 `const` 的 `lookup()` 中递增，无锁统计缓存命中率。这是 HFT 里最常见的 `mutable atomic` 用法——热路径上不能容忍锁。
- **延迟统计**：`mutable std::atomic<int64_t> last_latency_ns;` 在 `const` 的 `process()` 中记录延迟——不影响"逻辑 const"但必须线程安全。
- **注意缓存一致性**：每个 `atomic` 变量自身是原子的，但"缓存值 + 版本"要一起更新就得上锁；否则观测线程会看到新值配旧版本。

---

## 自测题

**1. `const` 成员函数为什么可能不是线程安全的？`mutable` 在其中扮演什么角色？**
<details><summary>答案</summary>
`const` 成员函数只保证"不修改对象的**逻辑**状态"；编译器层面它只是把 `this` 变成 `const T*`，从而禁止修改非 mutable 成员，它**完全不提供**任何线程安全保证。一旦函数通过 `mutable` 成员（缓存、计数器、延迟统计）或指针/引用间接写共享数据，多个线程同时调用这个 `const` 函数就是在并发写同一份数据，产生 data race。`mutable` 正是这个后门：让"看起来只读"的函数实际上在写状态，把并发风险藏在 const 的外衣下。
</details>

**2. 修复 `const` 函数的数据竞争有哪两种方式？各自适用什么场景？**
<details><summary>答案</summary>
① **`std::atomic`**（无锁）：把成员改成 `mutable std::atomic<T>`，适用于**单个变量**的读-改-写（计数器、命中 bool、指针交换），开销最小、不阻塞，HFT 热路径首选。② **`std::mutex`**（加锁）：在 `const` 函数里用 `std::lock_guard` 保护临界区，适用于需要保护**多个变量**或维持跨变量不变量的场景（缓存值 + 时间戳 + 计数器必须一致），代价是阻塞与上下文切换。多读少写的缓存还可考虑 `std::shared_mutex`。
</details>

**3. 为什么 `mutex` 也要声明为 `mutable`？**
<details><summary>答案</summary>
在 `const` 成员函数里 `this` 的类型是 `const T*`，所有非 mutable 成员都变成 `const`，而 `mutex::lock()` 是**非 const** 成员函数——不加 `mutable` 就没法在 `const` 函数里加锁。把锁声明为 `mutable std::mutex m_;` 是在表达"锁本身不影响对象的逻辑状态，加锁只是实现细节"，这与 `mutable` 的语义正好吻合。同理 `mutable std::atomic<...>` 也是这个道理。
</details>

**4. `std::atomic` 能替代 `std::mutex` 保护多个变量的复合操作吗？**
<details><summary>答案</summary>
不能。`std::atomic` 只保证**单个对象**上的读、写、读-改-写（如 `fetch_add`、`compare_exchange`）是原子的，并提供内存序控制；它无法让"对多个变量的一组操作"整体表现为原子——其他线程可能观察到中间状态。复合操作（缓存值 + 时间戳 + 计数器必须一致）仍然需要 `std::mutex`，或把所有不变量压缩进一个 atomic（例如用打包的序号/版本做校验）。
</details>

**5. 实测中两个线程各加 100 万次，为什么结果接近 100 万而不是接近 200 万？**
<details><summary>答案</summary>
`++count` 在 arm64 上是"读-改-写"三条指令：两个核几乎同时从内存读到同一个旧值，各自加一，再各自写回，于是两次自增只生效一次。两个线程在同一缓存行上反复互相使无效，绝大多数更新被覆盖，结果就落在 100 万附近。重点不是"少了多少"，而是**每次跑都不一样**——未定义行为的输出没有稳定值，这正是它比普通 bug 危险的地方。
</details>

**6. 下面代码有什么问题？**
```cpp
class Counter {
    mutable int count = 0;
public:
    int get() const { return ++count; }
};
```
<details><summary>答案</summary>
数据竞争：`count` 是普通 `int`，`++count` 是"读-改-写"三步，多线程并发调用 `get()` 属未定义行为（丢更新、读到撕裂值），而 `const` 的外衣完全掩盖了这个风险。修正取决于需求：无锁计数用 `mutable std::atomic<int> count{0};` 配 `return count.fetch_add(1) + 1;`；若需要更强一致性则加 `mutable std::mutex m_;` 并用 `std::lock_guard<std::mutex> g(m_);` 保护。
</details>

**7. 给了 `std::atomic` 成员之后，类的拷贝/移动语义会发生什么？**
<details><summary>答案</summary>
`std::atomic` 的拷贝构造与拷贝赋值被显式 `= delete`（原子对象不可拷贝——拷贝不是原子的，语义上说不通）。于是含有 `atomic` 成员的类，其隐式拷贝构造/拷贝赋值也被定义为 deleted；同时移动操作也不会自动生成。实测报错：`error: call to implicitly-deleted copy constructor of 'OrderBook'` + `note: copy constructor ... is implicitly deleted because field ... has a deleted copy constructor`。需要拷贝就得自己写一个，把 `atomic` 用 `.load()` 取出后再存到新对象里。
</details>

---

## 参考与延伸

- 下一节：[Item 17 特殊成员函数生成规则](item17-special-member-functions.md)
- 回到：[第 3 章 移步现代 C++](README.md)
