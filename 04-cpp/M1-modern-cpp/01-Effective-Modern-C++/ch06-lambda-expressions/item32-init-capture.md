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

<details>
<summary>参考答案</summary>

1. 这是 C++14 的**初始化捕获**（init capture / generalized lambda capture）：等号左边 `sp` 是闭包类型的一个新数据成员的名字，右边 `std::move(sp)` 是在**外围作用域**求值的初始化表达式。语义是：用 `std::move(sp)` 初始化闭包的成员 `sp`，即把外层 `shared_ptr` 的所有权**转移进闭包**（只移动，不增加引用计数），闭包持有它直到闭包销毁。这样闭包能安全地在异步场景里延长对象生命周期，且避免了引用计数递增的原子开销。

2. 它解决了 C++11 捕获列表只能"按值拷贝"或"按引用"的两种固定形式，无法做到：①**移动捕获**（把 `unique_ptr`、大对象、`std::string` 移进闭包而不拷贝）；②**捕获任意表达式的结果**（`[x = compute()]`、`[p = std::make_shared<T>()]`），不必先在外层定义一个具名变量；③给捕获成员起一个与外层不同的名字（`[name = this->name_]`），从而避免 `[=]` 实际只捕获 `this` 的陷阱。

3. 常用变通是"**bind + 按值传参**"：用 `std::bind` 把要移动的对象作为实参塞进 bind 对象，再让 lambda 接收它——
```cpp
auto cb = std::bind([](std::unique_ptr<Widget>& p){ p->do_work(); }, std::move(pw));
```
或者更直观地：在外层先 `auto tmp = std::move(pw);`，再用一个 `shared_ptr` 或按值捕获的容器把资源"包"起来交给 lambda（代价是引用计数或额外拷贝）。C++14 引入初始化捕获后，这些变通都不再需要。

4. 崩溃（未定义行为）。`[sp = std::move(sp)]` 把外层 `sp` 的所有权**移进闭包**，外层的 `sp` 变成空的 `shared_ptr`（移动后的 shared_ptr 保证为空）。紧接着的 `sp->do_work()` 就是对一个空 `shared_ptr` 解引用——未定义行为，实际通常直接段错误。修正：如果想两边都能用，就按值**拷贝**捕获一份（`[sp]` 或 `[sp = sp]`，引用计数 +1）；如果本意就是把所有权交给闭包，则删掉后面那行，只在闭包内使用 `sp`。

</details>

---

## 参考与延伸

- 下一节：[Item 33 泛型 lambda](item33-generic-lambda.md)
- 回到：[第 6 章](README.md)
