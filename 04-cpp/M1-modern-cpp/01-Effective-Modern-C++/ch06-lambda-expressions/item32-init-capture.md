# Item 32：用初始化捕获将对象移入闭包（C++14）

> 第 6 章 Lambda 表达式 · Item 32 · 上一节：[Item 31 避免默认捕获](item31-avoid-default-capture.md)

## 为什么要学这个（先建立直觉）

C++11 的 lambda 捕获只有两种姿势：按值拷贝、按引用。这对 `int`、`double` 够用，但遇到 **move-only 类型**就死锁了——`std::unique_ptr`、`std::future`、`std::promise`、包装了大 buffer 的对象，全都不可拷贝。`[pw]` 编译失败，`[&pw]` 能编译但闭包逃逸后 `pw` 悬垂（[Item 31](item31-avoid-default-capture.md) 刚踩过这个坑）。你真正想要的是「把这个对象**移动**进闭包」，C++11 做不到。

C++14 的初始化捕获（init capture，也叫 generalized lambda capture）把捕获列表从「列变量名」升级成「写初始化语句」：`[up = std::move(pw)]`——等号右边是任意表达式，在 lambda **创建时**求值；左边是闭包里新数据成员的名字。移动捕获只是它能做的事情之一，它还能给捕获改名、捕获计算结果、捕获 `*this` 的副本。

这一节短，但它是现代 lambda 的分水岭：有了 init capture，lambda 的捕获能力才追平了（实际超越了）`std::bind`，Item 34 才能理直气壮地说「别用 bind 了」。

---

## 这节讲什么

1. **移动捕获** `[up = std::move(pw)]` 实测：`unique_ptr` 移入闭包，原指针变 `nullptr`；
2. **表达式捕获** `[n = compute()]`：求值时机在 lambda **创建时**而非调用时，用打印实测证明；
3. **C++11 的历史变通**：用 `std::bind` 模拟移动捕获（展示即可，C++14 后淘汰）；
4. init capture 的语法语义：右边在**外围作用域**求值，左边是闭包的**新成员**。

---

## 移动捕获：unique_ptr 进闭包

本机实测（clang 23.1.0，`-std=c++17 -Wall`）：

```cpp
#include <iostream>
#include <memory>

struct Widget {
    int id;
    void work() const { std::cout << "Widget " << id << " working\n"; }
};

int main() {
    auto pw = std::make_unique<Widget>(Widget{7});
    auto cb = [up = std::move(pw)] { up->work(); };
    std::cout << "移动后 pw == nullptr ? " << (pw == nullptr) << "\n";
    cb();
    return 0;
}
```

实测输出：

```
移动后 pw == nullptr ? 1
Widget 7 working
```

语义拆解：`[up = std::move(pw)]` 在闭包类型里声明了一个名为 `up` 的数据成员，用 `std::move(pw)` 初始化它——`up` 的类型按 `auto` 规则推导为 `std::unique_ptr<Widget>`。移动发生后，外层 `pw` 被掏空（实测为 `nullptr`），所有权进入闭包。闭包随 `std::function`、任务队列走到哪里，`Widget` 就活到哪里——这正是异步回调需要的所有权语义。

注意语法对称性的陷阱：`[up = std::move(pw)]` 里等号**右边在外围作用域求值**（所以 `pw` 指外层的 `pw`），**左边是闭包内部的新名字**（和外层没有任何关系，只是恰好可以同名）。

---

## 表达式捕获：创建时求值，不是调用时

`=` 右边可以是任意表达式。一个必须钉死的直觉：**表达式在 lambda 创建的那一刻求值一次**，之后每次调用用的都是那个已存好的值：

```cpp
#include <iostream>

int compute() {
    std::cout << "compute() 被调用\n";
    return 42;
}

int main() {
    std::cout << "--- 创建 lambda 前 ---\n";
    auto cb2 = [n = compute()] { return n * 2; };
    std::cout << "--- lambda 已创建，尚未调用 ---\n";
    std::cout << "cb2() = " << cb2() << "\n";
    std::cout << "cb2() = " << cb2() << "（compute 没有再次执行）\n";
    return 0;
}
```

本机实测（clang 23.1.0）输出：

```
--- 创建 lambda 前 ---
compute() 被调用
--- lambda 已创建，尚未调用 ---
cb2() = 84
cb2() = 84（compute 没有再次执行）
```

`compute()` 只在创建时跑了一次——「`compute() 被调用`」出现在「lambda 已创建」**之前**，两次 `cb2()` 都没有触发新的计算。这和函数参数「调用时求值」的直觉相反，写错时机（比如把本该每次调用都重算的表达式放进 init capture）会悄悄拿到过期值。

更多形态：

```cpp
// 捕获 this 所指对象的副本（C++17 起更推荐 [*this]）
auto cb = [self = *this] { return self.data; };

// 按值捕获 shared_ptr：引用计数 +1，闭包保活共享对象
auto sp = std::make_shared<Config>();
auto cb2 = [config = sp] { return config->get("key"); };

// 捕获引用（少见，等价于 [&timer]）
auto cb3 = [&timer = timer] { return timer.elapsed(); };
```

---

## C++11 的历史变通：std::bind

C++11 没有 init capture，想把 `unique_ptr` 塞进可调用对象只能用 `std::bind`：把要移动的对象作为实参移进 bind 对象，lambda 按引用接收：

```cpp
#include <functional>
#include <iostream>
#include <memory>

struct Widget {
    int id;
    void work() const { std::cout << "Widget " << id << " working\n"; }
};

int main() {
    std::unique_ptr<Widget> pw(new Widget{7});   // C++11 没有 make_unique
    auto cb = std::bind([](std::unique_ptr<Widget>& w) { w->work(); },
                        std::move(pw));
    std::cout << "移动后 pw == nullptr ? " << (pw == nullptr) << "\n";
    cb();
    return 0;
}
```

本机实测（clang 23.1.0，`-std=c++11 -Wall`）输出：

```
移动后 pw == nullptr ? 1
Widget 7 working
```

能跑，但写法绕：要理解 bind 的实参存储语义、占位符规则（[Item 34](item34-lambda-vs-bind.md) 细讲），还引入了 bind 自身的坑（默认拷贝、不可内联）。顺带实测：C++11 模式下直接写 init capture，clang 23.1.0 报（`-std=c++11 -pedantic-errors`）：

```
error: initialized lambda captures are a C++14 extension
      [-Werror,-Wc++14-extensions]
    auto cb = [y = x * 2] { return y; };
               ^
```

（不加 `-pedantic-errors` 时 clang 只给扩展警告并放行——这是编译器兼容行为，不是标准许可。）C++14 之后，bind 的这个历史使命正式结束。

---

## 完整可运行例子

把两个核心用法放一个程序里（就是上文两段实测代码的合体）：

```cpp
#include <iostream>
#include <memory>

struct Widget {
    int id;
    void work() const { std::cout << "Widget " << id << " working\n"; }
};

int compute() {
    std::cout << "compute() 被调用\n";
    return 42;
}

int main() {
    // 1. 移动捕获 unique_ptr
    auto pw = std::make_unique<Widget>(Widget{7});
    auto cb = [up = std::move(pw)] { up->work(); };
    std::cout << "移动后 pw == nullptr ? " << (pw == nullptr) << "\n";
    cb();

    // 2. 表达式捕获：创建时求值
    std::cout << "--- 创建 lambda 前 ---\n";
    auto cb2 = [n = compute()] { return n * 2; };
    std::cout << "--- lambda 已创建，尚未调用 ---\n";
    std::cout << "cb2() = " << cb2() << "\n";
    std::cout << "cb2() = " << cb2() << "（compute 没有再次执行）\n";
    return 0;
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）输出：

```
移动后 pw == nullptr ? 1
Widget 7 working
--- 创建 lambda 前 ---
compute() 被调用
--- lambda 已创建，尚未调用 ---
cb2() = 84
cb2() = 84（compute 没有再次执行）
```

---

## 常见错误（新手踩坑）

1. **移动后继续使用原对象**。`[up = std::move(pw)]` 之后 `pw` 是 `nullptr`（实测），再 `pw->work()` 是 UB，实际通常直接崩溃。修正：移动后把原变量当不存在。
2. **以为表达式在调用时求值**。`[n = compute()]` 的 `compute()` 只在创建 lambda 时跑一次（实测打印顺序为证）。需要每次调用重算的逻辑要放进 lambda 体内，而不是捕获列表。
3. **给捕获起和外层相同的名字后以为改的是外层变量**。`[x = x]{ ++x; }`（需 `mutable`）改的是闭包里的副本，外层 `x` 纹丝不动——init capture 是按值存储的，除非显式 `[&x = x]`。
4. **在 C++11 代码里写 init capture**。编译报 `initialized lambda captures are a C++14 extension`（实测原文）；要么升级标准，要么退回 `std::bind` 变通。

---

## HFT 关联

- **把重资源移进异步任务**：`std::async([buf = std::move(large_buffer)] { flush(buf); })`——日志缓冲、行情快照这类大对象零拷贝进入后台任务，主线程交出所有权后继续跑热路径。
- **策略对象生命周期托管给闭包**：`[engine = std::move(engine_ptr)] { engine->on_tick(t); }` 让回调持有策略的 `unique_ptr`，配合 [Item 31](item31-avoid-default-capture.md) 的教训，彻底消除 `this` 悬垂。
- **捕获时固定快照**：`[threshold = risk.current_threshold()]` 在创建闭包时冻结风控阈值——创建时求值的语义恰好是「快照」，盘中改配置不影响已派发的任务。
- **shared_ptr 按值捕获的代价**：`[sp]` 引用计数 +1 是原子操作，热路径上每微秒都计数；能 move 就 move，能拷裸值就别拷智能指针。

---

## 自测题

**1. `[up = std::move(pw)]` 的语义是什么？等号两边分别在哪个作用域求值/生效？**
<details><summary>答案</summary>
这是 C++14 的初始化捕获：等号右边 `std::move(pw)` 是在**外围作用域**求值的初始化表达式（在 lambda 创建时执行一次）；等号左边 `up` 是闭包类型的**新数据成员**的名字，类型按 `auto` 规则推导（此处为 `std::unique_ptr<Widget>`）。语义是把外层 `unique_ptr` 的所有权转移进闭包——实测移动后外层 `pw == nullptr`。
</details>

**2. init capture 解决了 C++11 lambda 的哪些限制？**
<details><summary>答案</summary>
三个：①**移动捕获**——C++11 只有按值拷贝和按引用两种捕获，`unique_ptr` 等 move-only 类型进不了闭包；②**捕获任意表达式的结果**——`[n = compute()]`、`[p = std::make_shared<T>()]`，不必先在外层造一个具名变量；③**给捕获改名**——`[d = this->data]`，绕开 `[=]` 实际捕获 `this` 的陷阱（[Item 31](item31-avoid-default-capture.md)）。
</details>

**3. `[n = compute()]` 的 `compute()` 什么时候执行？每次调用 lambda 都会重算吗？**
<details><summary>答案</summary>
在 lambda **创建时**求值一次，结果存为闭包的数据成员；之后每次调用用的都是这个存好的值，不会重算。本文实测：`compute()` 的打印出现在「lambda 已创建，尚未调用」之前，两次 `cb2()` 都返回 84 且没有新的 `compute()` 打印。这和函数参数「调用时求值」的直觉相反。
</details>

**4. C++11 没有 init capture 时，怎么把 `unique_ptr` 移进可调用对象？**
<details><summary>答案</summary>
用 `std::bind`：`std::bind([](std::unique_ptr<Widget>& w){ w->work(); }, std::move(pw))`——把 `unique_ptr` 移进 bind 对象的实参存储，lambda 按引用接收它。本机 `-std=c++11` 实测可行（移动后 `pw == nullptr`，`cb()` 正常输出）。但写法绕、有 bind 自身的坑，C++14 起被 init capture 全面取代（见 [Item 34](item34-lambda-vs-bind.md)）。
</details>

**5. 下面代码有什么问题？**

```cpp
auto sp = std::make_shared<Widget>();
auto cb = [sp = std::move(sp)]{ sp->do_work(); };
sp->do_work();
```
<details><summary>答案</summary>
崩溃/UB。`[sp = std::move(sp)]` 把外层 `shared_ptr` 的所有权**转移**进闭包（移动后的 `shared_ptr` 保证为空），紧接着的 `sp->do_work()` 对空 `shared_ptr` 解引用——未定义行为，实际通常段错误。修正：想两边都用就按值**拷贝**捕获（`[sp]`，引用计数 +1）；本意是移交所有权就删掉最后一行。
</details>

**6. init capture 和 `[*this]`（C++17）都能捕获对象的副本，怎么选？**
<details><summary>答案</summary>
`[*this]` 拷贝**整个对象**进闭包，一行搞定但粒度粗；`[d = data]` 只拷需要的成员，对象大时更省。成员少/对象小用 `[*this]` 最简洁；只缺一两个成员、或想给捕获改个不与成员冲突的名字时用 init capture。C++14/11 代码没有 `[*this]`，只能用 `[self = *this]`（C++14）或先拷局部（C++11）。
</details>

---

## 参考与延伸

- 下一节：[Item 33 泛型 lambda](item33-generic-lambda.md)
- 回到：[第 6 章 Lambda 表达式](README.md)
