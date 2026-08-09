# Item 32：用初始化捕获将对象移入闭包（C++14）

> 第 6 章 · Item 32 · 上一节：[Item 31 避免默认捕获](item31-avoid-default-capture.md)

## 为什么要学这个（先建立直觉）

C 的回调通过 `void*` 传递上下文，可以传任何东西（包括 `malloc` 的内存）：

```c
void callback(void* ctx) {
    Widget* w = (Widget*)ctx;
    w->do_work();
}
void* ctx = malloc(sizeof(Widget));
widget_init((Widget*)ctx);
register_callback(callback, ctx);  // 传递堆分配的对象
```

C++11 的 lambda 捕获只能拷贝或引用，**不能移动**：

```cpp
auto pw = std::make_unique<Widget>();
// C++11：无法把 pw 移动进闭包！
auto cb = [pw]{ pw->do_work(); };  // 编译失败！unique_ptr 不可拷贝
auto cb2 = [&pw]{ pw->do_work(); };  // 能编译但 pw 可能悬垂
```

C++14 的初始化捕获（init capture）解决了这个问题——允许在捕获时执行表达式并命名，包括 `std::move`：

```cpp
auto pw = std::make_unique<Widget>();
auto cb = [up = std::move(pw)]{ up->do_work(); };  // 移动进闭包！
```

---

## 这节讲什么

初始化捕获（init capture）能在捕获时执行表达式并命名——彻底解决"想捕获移动语义"的需求，C++11 做不到。

---

## 核心用法

### 移动捕获

```cpp
auto pw = std::make_unique<Widget>();
auto cb = [up = std::move(pw)]{ up->doSomething(); };
// up = std::move(pw) 在闭包里创建 up（按值，即移动），pw 被掏空
// up 是闭包成员，类型推导为 unique_ptr<Widget>
// pw 现在是 nullptr
```

`[up = std::move(pw)]` 的含义：在闭包里创建 `up`，用 `std::move(pw)` 初始化它。`up` 是闭包成员，类型推导为 `unique_ptr<Widget>`。

### 表达式捕获

```cpp
// 捕获表达式的结果
auto cb = [x = compute_value()]{ return x * 2; };
// compute_value() 的结果被存在闭包里

// 捕获 this 的拷贝（C++17 更好的方式是 [*this]）
auto cb2 = [self = *this]{ return self.data; };

// 捕获 shared_ptr
auto sp = std::make_shared<Config>();
auto cb3 = [config = sp]{ return config->get("key"); };
```

### C++11 变通：std::bind

```cpp
// C++11 没有 init capture，用 bind 变通
auto pw = std::make_unique<Widget>();
auto cb = std::bind([](std::unique_ptr<Widget>& w){ w->doSomething(); },
                    std::move(pw));
// 更绕，且 bind 的值传递语义对 unique_ptr 有其他问题
```

C++11 的变通是 `std::bind`，但更绕。

---

## 常见错误（新手踩坑）

**错误 1：C++11 代码尝试移动捕获**
```cpp
// C++11：编译失败
auto pw = std::make_unique<Widget>();
auto cb = [pw = std::move(pw)]{ ... };  // C++14 才支持
```
**修正：** 升级到 C++14 或用 `std::bind` 变通。

**错误 2：移动后还用原对象**
```cpp
auto pw = std::make_unique<Widget>();
auto cb = [up = std::move(pw)]{ up->doSomething(); };
pw->doSomething();  // pw 是 nullptr！UB
```
**修正：** 移动后不要用原对象。

**错误 3：忘了 init capture 是按值存储**
```cpp
int x = 42;
auto cb = [x = x]{ return x; };  // 拷贝 x 到闭包
// 外部 x 改变不影响闭包内的 x
x = 100;
cb();  // 返回 42，不是 100
```
**修正：** 理解 init capture 是按值存储（除非显式用引用 `&x = x`）。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 回调上下文 | `void*` userdata | lambda 捕获 | 类型安全 |
| 移动捕获 | 手动 `malloc`+传递 | `[up = std::move(pw)]` | C++14 |
| 表达式捕获 | 不适用 | `[x = expr]` | C++14 |
| 所有权转移 | 手动管理 | unique_ptr + init capture | RAII |

**一句话总结：** C 程序员记住——C++14 的 init capture 让你把 `unique_ptr` 等不可拷贝的对象移动进闭包。语法是 `[name = expression]`。

---

## HFT 关联

- **移动资源进闭包**：策略对象 `move` 进闭包，避免拷贝大对象。
- **异步配置**：`auto cb = [config = std::move(strategy_config)]{ ... }` 移动配置进异步任务。
- **unique_ptr 回调**：`[up = std::move(widget_ptr)]{ up->on_tick(tick); }` 移动智能指针进闭包，确保生命周期。

---

## 自测题

1. `[up = std::move(pw)]` 的语义是什么？
2. 初始化捕获解决了 C++11 的什么限制？
3. C++11 没有 init capture 时怎么变通？
4. 下面代码有什么问题？
```cpp
auto sp = std::make_shared<Widget>();
auto cb = [sp = std::move(sp)]{ sp->do_work(); };
sp->do_work();
```

---

## 参考与延伸

- 下一节：[Item 33 泛型 lambda](item33-generic-lambda.md)
- 回到：[第 6 章](README.md)
