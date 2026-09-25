# Item 16：让 const 成员函数线程安全

> 第 3 章 移步现代 C++ · Item 16 · 上一节：[Item 15 constexpr](item15-constexpr.md)

## 为什么要学这个（先建立直觉）

C 没有 `const` 成员函数。C 程序员用 `const` 修饰指针或变量：

```c
struct Cache {
    int value;
    int cached;
};

// C 的"const 方法"模拟——传 const 指针
int cache_get(const struct Cache* c) {
    // c->cached = 1;  // 编译失败！c 是 const
    return c->value;    // 只能读
}
```

C++ 的 `const` 成员函数承诺"不修改对象状态"。但有个后门——`mutable` 关键字：

```cpp
class Cache {
    mutable int cachedValue;    // mutable：const 函数也能改
public:
    int getValue() const {       // const 函数
        if (!cachedValue)
            cachedValue = compute();  // 修改 mutable 成员！
        return cachedValue;
    }
};
```

**问题来了：** 如果两个线程同时调 `getValue()`，它们都在修改 `cachedValue`——这是数据竞争（data race）。`const` 函数不代表线程安全！C++ 标准假设 `const` 成员函数可能被并发调用（除非有其他同步措施），所以如果 `const` 函数修改了 `mutable` 成员，你必须自己加锁或用 `atomic`。

---

## 这节讲什么

`const` 成员函数仍可修改 `mutable` 成员（如缓存、互斥锁）。如果 `const` 函数会读写 `mutable` 成员，它就不是天然线程安全的——必须加锁或用 `std::atomic`。

---

## 核心问题

### const 函数的数据竞争

```cpp
class Cache {
    mutable int cachedValue;
public:
    int getValue() const {       // const 函数
        if (!cachedValue) cachedValue = compute();  // 修改 mutable 成员！
        return cachedValue;
    }
};
// 两个线程同时调 getValue() → 数据竞争（cachedValue 的读-改-写不是原子的）
// 结果：可能 compute() 被调用两次，或 cachedValue 写入撕裂
```

### 修复方案一：std::atomic

```cpp
class Cache {
    mutable std::atomic<int> cachedValue;
    mutable std::atomic<bool> cacheValid{false};
public:
    int getValue() const {
        if (!cacheValid.load()) {          // 原子读
            int val = compute();
            cachedValue.store(val);         // 原子写
            cacheValid.store(true);
        }
        return cachedValue.load();
    }
};
// 适用场景：单个原子变量的读写，无复杂逻辑
```

### 修复方案二：std::mutex

```cpp
class Cache {
    mutable std::mutex m;
    mutable int cachedValue = 0;
    mutable bool valid = false;
public:
    int getValue() const {
        std::lock_guard<std::mutex> lock(m);   // 加锁
        if (!valid) {
            cachedValue = compute();
            valid = true;
        }
        return cachedValue;
    }
};
// 适用场景：需要保护多个变量的复合操作
// 注意：mutex 必须是 mutable——const 函数需要能锁它
```

---

## 常见错误（新手踩坑）

**错误 1：以为 const = 线程安全**
```cpp
class Data {
    mutable std::vector<int> cache;
public:
    const std::vector<int>& get() const {
        if (cache.empty()) cache = load();  // 修改 mutable 成员！
        return cache;                       // 多线程并发 → 数据竞争
    }
};
```
**修正：** 用 `mutex` 或 `atomic` 保护 `mutable` 成员。

**错误 2：mutex 没声明 mutable**
```cpp
class Cache {
    std::mutex m;     // 不是 mutable！
    int getValue() const {
        std::lock_guard<std::mutex> lock(m);  // 编译失败！const 函数不能修改 m
    }
};
```
**修正：** `mutable std::mutex m;`——锁必须在 const 函数中可修改。

**错误 3：用 atomic 做复合操作**
```cpp
class Bad {
    mutable std::atomic<bool> valid{false};
    mutable std::atomic<int> value{0};
public:
    int get() const {
        if (!valid.load()) {
            value.store(compute());  // 两个独立的原子操作
            valid.store(true);       // 中间可能被其他线程插入！
        }
        return value.load();
    }
};
```
**修正：** 多个变量的复合操作必须用 `mutex`，`atomic` 只保护单个变量。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| const 方法 | 传 `const` 指针 | `const` 成员函数 | 面向对象封装 |
| mutable | 不存在 | `mutable` 关键字 | 允许 const 函数修改"不影响逻辑状态"的成员 |
| 线程安全 | 手动加 `pthread_mutex` | `std::mutex` 或 `std::atomic` | C++11 标准库 |
| 锁的声明 | 手动管理 | `mutable std::mutex` | const 函数需要能锁 |

**一句话总结：** C 程序员记住——C++ 的 `const` 成员函数不是"只读"的绝对保证，`mutable` 开了后门。如果 `const` 函数修改了 `mutable` 成员，多线程并发调用必须加锁或用 `atomic`。

---

## HFT 关联

- **行情缓存**：`const` 的 `get_tick()` 如果内部更新 `mutable` 缓存，必须用 `atomic` 或 `mutex` 保护——否则多策略线程并发读会 data race。
- **原子计数器**：`mutable std::atomic<uint64_t> hit_count;` 在 `const` 的 `lookup()` 函数中递增——无锁统计缓存命中率。
- **延迟统计**：`mutable std::atomic<int64_t> last_latency_ns;` 在 `const` 的 `process()` 中记录延迟——不影响"逻辑 const"但需要线程安全。

---

## 自测题

1. `const` 成员函数为什么可能不是线程安全的？`mutable` 在其中扮演什么角色？
2. 修复 `const` 函数的数据竞争有哪两种方式？各自适用什么场景？
3. 为什么 `mutex` 也要声明为 `mutable`？
4. `std::atomic` 能替代 `std::mutex` 保护多个变量的复合操作吗？
5. 下面代码有什么问题？
```cpp
class Counter {
    mutable int count = 0;
public:
    int get() const { return ++count; }
};
```

<details>
<summary>参考答案</summary>

1. `const` 成员函数只保证"不修改对象的**逻辑**状态"，编译器层面它只是把 `this` 变成 `const T*`，从而禁止修改非 mutable 成员；它**完全不提供**任何线程安全保证。一旦函数通过 `mutable` 成员（缓存、计数器、延迟统计）或指针/引用间接修改了共享数据，多个线程同时调用这个 `const` 函数就是在并发写同一份数据，产生 data race。`mutable` 正是这个后门：它让"看起来只读"的函数实际上在写状态，把并发风险隐藏在 const 的外衣下。

2. 两种：①**`std::atomic`**（无锁）：把被修改的成员改成 `mutable std::atomic<T>`，适用于单个变量的读-改-写（计数器、缓存命中的 bool 标记、指针交换），开销最小、不会阻塞，HFT 热路径首选。②**`std::mutex`**（加锁）：在 const 函数里用 `std::lock_guard` 保护临界区，适用于需要保护**多个变量**或需要维持跨变量不变量的场景（如同时更新缓存值和版本号），代价是阻塞与上下文切换开销。另可考虑读写锁（`std::shared_mutex`）保护"多读少写"的缓存。

3. 因为在 `const` 成员函数里，`this` 的类型是 `const T*`，所有非 mutable 成员都变成 `const`，而 `mutex::lock()` 是非 const 成员函数——不加 `mutable` 就没法在 const 函数里加锁。把锁声明为 `mutable std::mutex m_;` 是在表达"锁本身不影响对象的逻辑状态，加锁只是实现细节"，这与 `mutable` 的语义正好吻合。同理 `mutable std::atomic<...>` 也是这个道理（`atomic::fetch_add` 需要非 const 对象）。

4. 不能。`std::atomic` 只保证**单个对象**上的读、写、读-改-写（如 `fetch_add`、`compare_exchange`）是原子的，并提供内存序控制；它无法让"对多个变量的一组操作"整体表现为原子——其他线程可能观察到中间状态，破坏跨变量的不变量。复合操作（如"更新缓存值 + 更新时间戳 + 递增计数器"必须一致）仍然需要 `std::mutex`（或把所有不变量压缩进一个 atomic，如用一个原子打包的结构/序号做版本控制）。

5. 数据竞争：`count` 是普通 `int`，`++count` 编译成"读-改-写"三条指令，多个线程并发调用 `get()` 时是未定义行为（可能丢更新、读到撕裂值），而且 `const` 的外衣完全掩盖了这个风险。正确写法取决于需求：无锁计数用 `mutable std::atomic<int> count{0};` 配 `return count.fetch_add(1) + 1;`；若需要更强的一致性则加 `mutable std::mutex m_;` 并用 `std::lock_guard<std::mutex> g(m_);` 保护。

</details>

---

## 参考与延伸

- 下一节：[Item 17 特殊成员函数生成规则](item17-special-member-functions.md)
- 回到：[第 3 章 移步现代 C++](README.md)
