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

---

## 参考与延伸

- 下一节：[Item 17 特殊成员函数生成规则](item17-special-member-functions.md)
- 回到：[第 3 章 移步现代 C++](README.md)
