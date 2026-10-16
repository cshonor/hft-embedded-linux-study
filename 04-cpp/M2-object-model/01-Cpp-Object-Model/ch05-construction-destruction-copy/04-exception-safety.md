# 5.4 异常安全

> 第 5 章 · 上一节：[5.3 new/delete 的两步](03-new-delete.md) · 下一章：[第 6 章 运行时语义](../ch06-runtime-semantics/README.md)

## 这节讲什么

构造函数抛异常时内存不泄漏（RAII），但析构函数抛异常危险（`terminate`）。析构函数应 `noexcept`。RAII 是 C++ 异常安全的核心保障。

---

## 为什么要学这个（先建立直觉）

C 程序员用错误码处理失败，没有异常的概念：

```c
// C：错误码处理
Widget* create_widget() {
    void* mem = malloc(sizeof(Widget));
    if (!mem) return NULL;  // 错误码
    if (init_widget(mem) != 0) {
        free(mem);  // 手动回滚
        return NULL;
    }
    return mem;
}
// 调用者必须检查返回值，容易遗漏
```

C++ 的异常让"失败传播"自动化——但如果构造函数中途抛异常，已分配的资源怎么办？

```cpp
Widget* p = new Widget(args);
// 如果 Widget 构造抛异常：
// 1. 已构造的成员/基类按逆序析构（RAII）
// 2. operator new 分配的内存自动释放
// → 内存不泄漏（RAII 保证）
```

关键洞察：**C++ 的异常 + RAII 让资源管理比 C 的手动回滚更安全。**

---

## 异常安全详解

### 构造函数抛异常 → 安全

```cpp
class Widget {
    std::string name;    // 成员1
    int* data;           // 成员2
public:
    Widget() : name("test"), data(new int[100]) {
        // 如果 name 构造成功，data 分配成功，
        // 但这里抛异常 → name 自动析构，data 泄漏！
        throw std::runtime_error("init failed");
    }
    // 问题：data 是裸指针，异常时不会自动释放
};
// 修正：用 unique_ptr 管理 data → 异常时自动释放
```

### 析构函数抛异常 → 灾难

```cpp
class Bad {
public:
    ~Bad() {
        if (error) throw std::runtime_error("dtor failed");
        // 如果正在栈展开（另一个异常正在传播）
        // 析构再抛异常 → std::terminate → 程序崩溃
    }
};
```

### RAII 保证

```cpp
class Good {
    std::unique_ptr<int[]> data;  // 智能指针管理
    std::string name;
public:
    Good() : data(std::make_unique<int[]>(100)), name("test") {
        // 如果这里抛异常：
        // data 已构造 → 自动析构（释放内存）
        // name 已构造 → 自动析构
        // → 零泄漏
    }
    ~Good() noexcept = default;  // 析构不抛异常
};
```

---

## 常见错误（新手踩坑）

### 错误 1：裸指针成员 + 构造抛异常 = 泄漏

```cpp
class Leaky {
    int* a;
    int* b;
public:
    Leaky() : a(new int[100]), b(new int[100]) {
        throw std::runtime_error("fail");
        // a 和 b 都泄漏！析构不会被调用（对象没构造完成）
    }
    ~Leaky() { delete[] a; delete[] b; }
    // 析构没被调，因为构造没完成
};
// 修正：用 unique_ptr
```

### 错误 2：析构抛异常

```cpp
class Bad {
public:
    ~Bad() {
        close(fd);  // close 可能失败
        if (close_failed) throw std::runtime_error("close failed");
        // 如果在栈展开期间 → terminate → 崩溃
    }
};
// 修正：~Bad() noexcept { try { close(fd); } catch(...) {} }
```

### 错误 3：异常安全等级不够

```cpp
class Account {
    int balance;
public:
    void transfer(Account& to, int amount) {
        balance -= amount;      // 步骤1
        to.balance += amount;   // 步骤2：如果这里抛异常 → balance 已减但 to 没加
    }
};
// 修正：先拷贝，再交换（copy-and-swap 惯用法）
```

---

## 和 C 的区别

| 特性 | C 错误码 | C++ 异常 |
|------|---------|---------|
| 失败传播 | 手动检查返回值 | 自动传播（栈展开） |
| 资源清理 | 手动 goto cleanup | RAII 自动析构 |
| 构造失败 | 返回 NULL | 抛异常 + 自动回滚 |
| 析构失败 | 手动处理 | **绝不抛异常**（terminate） |
| 正常路径开销 | 每次检查 if | **零开销**（table-based EH） |

---

## HFT 关联

1. **析构 noexcept**：HFT 析构绝不抛异常（否则 terminate 拉崩进程）。所有析构函数默认 noexcept（C++11 起）。
2. **异常当致命错误**：HFT 把异常当"不可恢复错误"用（崩溃重启），不当控制流。热路径用错误码。
3. **RAII 保证零泄漏**：用智能指针（unique_ptr/shared_ptr）管理资源——即使异常也不泄漏。

---

## 代码自测

### Q1: 构造抛异常

```cpp
class Widget {
    std::string name;
    int* data;
public:
    Widget() : name("ok"), data(new int[100]) {
        throw std::runtime_error("fail");
    }
    ~Widget() { delete[] data; }
};
try {
    Widget* w = new Widget();
} catch (...) {}
// data 泄漏了吗？name 泄漏了吗？
```

<details>
<summary>答案与复习指引</summary>

data 泄漏了（裸指针，析构没被调——构造没完成）。name 没泄漏（string 有析构，成员已构造的部分自动析构）。`operator new` 分配的内存也自动释放。修正：用 `unique_ptr<int[]>` 管理 data。

**复习：** → [5.4 异常安全](./04-exception-safety.md)
</details>

### Q2: 析构抛异常

```cpp
class Bad {
public:
    ~Bad() { throw std::runtime_error("oops"); }
};
void foo() {
    Bad b;
    throw std::runtime_error("first exception");
}
// foo() 抛异常时会发生什么？
```

<details>
<summary>答案与复习指引</summary>

`std::terminate` → 程序崩溃。`foo()` 抛 "first exception" 触发栈展开 → `b` 析构 → 析构再抛 "oops" → 两个异常同时存在 → C++ 调 `std::terminate`。**析构函数绝不抛异常。**

**复习：** → [5.4 异常安全](./04-exception-safety.md)
</details>

### Q3: RAII

```cpp
class Safe {
    std::unique_ptr<int[]> data;
    std::string name;
public:
    Safe() : data(std::make_unique<int[]>(100)), name("test") {
        throw std::runtime_error("fail");
    }
};
try {
    Safe s;
} catch (...) {}
// 有泄漏吗？为什么？
```

<details>
<summary>答案与复习指引</summary>

没有泄漏。`data` 和 `name` 都已构造（是智能指针/string），抛异常时已构造的成员自动析构——unique_ptr 释放内存，string 释放内部缓冲。这就是 RAII 的价值——异常安全，零泄漏。

**复习：** → [5.4 异常安全](./04-exception-safety.md)
</details>

### Q4: HFT 异常策略

```cpp
// HFT 热路径：用错误码
int process_order(Order& o) {
    if (o.qty <= 0) return -1;  // 错误码
    // ...
    return 0;
}

// HFT 初始化/致命错误：用异常
void init_engine() {
    if (!load_config()) {
        throw std::runtime_error("config load failed");
        // 崩溃重启策略
    }
}
// 为什么这样分？
```

<details>
<summary>答案与复习指引</summary>

热路径用错误码：①零开销（异常正常路径虽零开销但抛异常极慢）；②确定性（无栈展开）；③可预测延迟。初始化用异常：①初始化不在热路径（偶尔执行）；②异常适合"不可恢复错误"；③RAII 保证资源安全。HFT 策略：异常 = 致命错误（崩溃重启），错误码 = 可预期失败（重试/降级）。

**复习：** → [5.4 异常安全](./04-exception-safety.md)
</details>

---

## 参考与延伸

- 下一章：[第 6 章 运行时语义](../ch06-runtime-semantics/README.md)
- 回到：[第 5 章](README.md)
