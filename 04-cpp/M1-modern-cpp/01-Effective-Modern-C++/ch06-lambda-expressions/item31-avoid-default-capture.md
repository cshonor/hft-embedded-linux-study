# Item 31：避免默认捕获模式（[=] / [&]）

> 第 6 章 Lambda 表达式 · Item 31 · 下一节：[Item 32 初始化捕获](item32-init-capture.md)

## 为什么要学这个（先建立直觉）

C 用函数指针 + `void*` userdata 做回调：

```c
// C 回调模式
void on_tick(void* ctx, const Tick* t) {
    Engine* self = (Engine*)ctx;
    self->process(t);
}
register_callback(on_tick, engine);  // 传函数指针 + 上下文
```

C++ 的 lambda 可以"捕获"外层变量，比 C 的 `void*` 更方便。但默认捕获 `[=]`（按值）和 `[&]`（按引用）有隐蔽的陷阱：

```cpp
class Widget {
    int data;
    auto make_cb() {
        return [=]{ return data; };  // 看着像"按值捕获 data"
        // 实际捕获的是 this 指针！data 通过 this->data 访问
        // Widget 析构后 → this 悬垂 → UB
    }
};
```

`[=]` 在成员函数中捕获成员变量时，实际捕获的是 `this` 指针（按引用！），不是成员的拷贝。这是 C++ lambda 最常见的坑。

---

## 这节讲什么

默认捕获有两大问题：`[=]` 对成员变量的捕获语义反直觉（实际捕获 `this`）；`[&]` 的悬垂引用风险。

---

## 两大问题

### 1. [=] 捕获 this 而非成员拷贝

```cpp
class Widget {
    int data;
public:
    auto make_cb() {
        return [=]{ return data; };  // 捕获的是 this，不是 data 的拷贝！
        // 等价于：return [this]{ return this->data; };
    }
};
// Widget 销毁后闭包仍调 this->data → 悬垂 → UB

// 正确做法：先拷贝到局部变量，再按值捕获
auto make_cb_safe() {
    int local_data = data;           // 先拷贝到局部
    return [local_data]{ return local_data; };  // 按值捕获局部变量
}

// C++17：用 [*this] 按值捕获对象本身
auto make_cb_cpp17() {
    return [*this]{ return data; };  // 按值拷贝整个 *this
}
```

`[=]` 看着像"按值捕获一切"，但成员变量实际捕获的是 `this` 指针——按引用！

### 2. [&] 悬垂引用

```cpp
std::function<int()> get_counter() {
    int count = 0;
    return [&] { return ++count; };  // 按引用捕获 count
    // get_counter 返回后 count 销毁 → 引用悬垂 → UB
}
auto f = get_counter();
f();  // 访问已销毁的 count → UB

// 正确做法：按值捕获
std::function<int()> get_counter_safe() {
    int count = 0;
    return [count]() mutable { return ++count; };  // 按值捕获，mutable 允许修改
}
```

按引用捕获的局部变量，闭包存活超过作用域时引用悬垂——UB。

---

## 常见错误（新手踩坑）

**错误 1：成员函数中 [=] 捕获成员变量**
```cpp
class Engine {
    Config config;
public:
    auto get_handler() {
        return [=](const Tick& t) { return t.price > config.threshold; };
        // config 实际通过 this->config 访问——this 可能悬垂！
    }
};
```
**修正：** `int threshold = config.threshold; return [threshold](const Tick& t) { ... };`

**错误 2：[&] 捕获局部变量后异步使用**
```cpp
void start_async() {
    int id = generate_id();
    std::async([&]{ process(id); });  // id 在 start_async 返回后销毁！
}
```
**修正：** `std::async([id]{ process(id); });`——按值捕获。

**错误 3：忘记 mutable**
```cpp
int count = 0;
auto counter = [count] { return ++count; };  // 编译失败！count 是 const
auto counter2 = [count]() mutable { return ++count; };  // OK
```
**修正：** 按值捕获且需要修改时加 `mutable`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 回调 | 函数指针 + void* | lambda 捕获 | 更类型安全 |
| 捕获方式 | 手动传 userdata | `[=]`/`[&]`/`[x]` | 语法糖 |
| 成员捕获 | 不适用 | `[this]`/`[*this]` | C++17 新增 |
| 悬垂风险 | 手动管理 | `[&]` 有自动悬垂风险 | 需要注意生命周期 |

**一句话总结：** C 程序员记住——显式列出捕获变量（`[x]`、`[&mtx]`），不用 `[=]`/`[&]` 默认模式。成员变量用 `[*this]`（C++17）或先拷贝到局部。

---

## HFT 关联

- **策略回调**：`engine.on_tick([this](const Tick& t){ ... })` 注意 `this` 生命周期——策略销毁后引擎仍调闭包 = 悬垂。
- **异步任务**：`std::async([config](const Tick& t){ ... })` 按值捕获配置，避免引用悬垂。
- **线程安全**：按引用捕获共享变量需要确保锁的生命周期——`[&mtx]` 如果 `mtx` 先销毁则 UB。

---

## 自测题

1. `[=]` 在成员函数里捕获成员变量时，实际捕获的是什么？为什么是隐患？
2. `[&]` 的悬垂风险是什么场景？
3. 异步保存的闭包为什么不能用 `[&]`？该用什么替代？
4. C++17 的 `[*this]` 解决了什么问题？
5. 下面代码有什么问题？
```cpp
std::function<void()> make_task() {
    int x = 42;
    return [&] { std::cout << x; };
}
auto t = make_task();
t();
```

---

## 参考与延伸

- 下一节：[Item 32 初始化捕获](item32-init-capture.md)
- 回到：[第 6 章 Lambda 表达式](README.md)
