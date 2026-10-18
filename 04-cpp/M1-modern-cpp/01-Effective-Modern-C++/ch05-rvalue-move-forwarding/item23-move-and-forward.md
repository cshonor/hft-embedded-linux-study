# Item 23：理解 std::move 和 std::forward

> 第 5 章 右值引用、移动语义与完美转发 · Item 23 · 下一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)

## 为什么要学这个（先建立直觉）

C 的赋值和传参都是拷贝——没有"移动"的概念。想"转移所有权"只能手动交接指针：

```c
void transfer(Buffer* dst, Buffer* src) {
    dst->data = src->data;   // 指针交接 O(1)
    dst->size = src->size;
    src->data = NULL;         // 原对象手动置空
    src->size = 0;
}
```

C++11 把这个模式语言化了：移动构造函数自动完成"交接+置空"，你只需用 `std::move` 标记"我允许你搬走它"：

```cpp
std::vector<int> v1 = {1, 2, 3, 4, 5};
std::vector<int> v2 = std::move(v1);  // O(1) 指针交接，v1 变空
```

但这里有一个几乎所有新手都会有的误解：**`std::move` 本身不移动任何东西**。它不是函数意义上的"移动"，而是一次 `static_cast<T&&>`——强制类型转换。真正的移动发生在**之后**的重载决议里：转成右值后，编译器才能选中移动构造。分不清"标记"和"动作"，就看不懂移动语义的任何一行代码。

`std::forward` 是同一个故事的另一半：它也是 cast，不过是**有条件的** cast——只有实参原本是右值时才转成右值。它存在的唯一理由就是完美转发。

---

## 这节讲什么

`std::move` 是无条件的右值转换，`std::forward` 是有条件的右值转换；两者都是 cast，自身零开销、不移动任何东西。对 `const` 对象 move 会静默退化为拷贝。

---

## std::move 自身不移动任何东西（实测）

用带打印的 `Tracer` 类观察"移动到底发生在哪一步"，本机实测（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
struct Tracer {
    std::string data;
    Tracer(std::string d) : data(std::move(d)) {}
    Tracer(const Tracer& t) : data(t.data) { std::cout << "拷贝构造\n"; }
    Tracer(Tracer&& t) noexcept : data(std::move(t.data)) { std::cout << "移动构造\n"; }
};

Tracer a("hello");
std::move(a);              // ← 仅转换，不接收结果
Tracer b = std::move(a);   // ← 移动发生在这里
```

实测输出：

```
-- 仅 std::move(a)，不接收结果 --
a.data = hello（原封不动）     ← move 自身什么都没做
-- 用 move 的结果去构造 --
移动构造                       ← 移动发生在构造时
a.data = （被掏空）
```

顺带一个有趣的实测细节：libc++ 给 `std::move` 标了 `[[nodiscard]]`，单独写 `std::move(a);` 会吃警告 `warning: ignoring return value of function declared with 'nodiscard'`——标准库在帮你确认"你是不是以为 move 会干什么"。

---

## 对 const 对象 move → 静默变拷贝

移动构造的形参是 `T&&`（非 const），绑定不了 `const T&&`——因为移动要**修改**源对象（掏空它），const 不允许。重载决议退而求其次选中 `const T&` 拷贝构造。本机实测：

```cpp
const Tracer c("const data");
Tracer d = std::move(c);   // const Tracer&& 绑定不到 Tracer&&
```

实测输出：

```
-- const 对象 move --
拷贝构造                ← 不是移动！
c.data = const data（还在！）
```

不报错、不警告，静默走拷贝。同理，没有移动构造的类型（C++98 遗留类）被 move 也是静默拷贝——这铺垫了 [Item 29](item29-move-not-exist.md) 的"假定移动不存在"。

---

## std::forward 是条件 move（实测）

`forward<T>` 查 `T` 是不是引用类型来决定转不转换：`T` 推为 `U&`（实参是左值）→ 保持左值；`T` 推为 `U`（实参是右值）→ 转右值。对比实验，本机实测：

```cpp
void target(std::string& s)  { std::cout << "  target(string&)  —— 左值重载\n"; }
void target(std::string&& s) { std::cout << "  target(string&&) —— 右值重载\n"; }

template<class T> void withForward(T&& x) { target(std::forward<T>(x)); }
template<class T> void withMove(T&& x)    { target(std::move(x)); }

std::string s = "lvalue";
withForward(s);                     // 左值 → 应走左值重载
withMove(s);                        // 错误：左值被强转右值
withForward(std::string("rvalue")); // 右值 → 右值重载
withMove(std::string("rvalue"));    // 右值 → 恰好相同
```

实测输出：

```
传入左值 s:
forward 版本:   target(string&)  —— 左值重载   ← forward 保住了左值性
move 版本:      target(string&&) —— 右值重载   ← move 把左值也转成了右值！
传入右值 std::string("rvalue"):
forward 版本:   target(string&&) —— 右值重载
move 版本:      target(string&&) —— 右值重载
```

对万能引用用 `move`，传左值时调用方的对象会被掏空——编译通过、不崩、数据悄悄丢。这是移动语义最隐蔽的事故类型，[Item 25](item25-move-vs-forward-usage.md) 专讲使用规则。

对照表：

| | `std::move` | `std::forward<T>` |
|---|------------|-------------------|
| 本质 | `static_cast<T&&>`（无条件） | `static_cast<T&&>`（`T` 非引用时才生效） |
| 运行时开销 | 0 | 0 |
| 语义 | "我允许你搬走它" | "保留实参的左右值性，原样转发" |
| 典型场景 | 右值引用形参、实现移动构造 | 万能引用形参转发 |

---

## 完整可运行例子

move 不动作 + const 退化拷贝，完整实测代码（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
#include <iostream>
#include <string>
#include <utility>

struct Tracer {
    std::string data;
    Tracer(std::string d) : data(std::move(d)) {}
    Tracer(const Tracer& t) : data(t.data) { std::cout << "拷贝构造\n"; }
    Tracer(Tracer&& t) noexcept : data(std::move(t.data)) { std::cout << "移动构造\n"; }
};

int main() {
    Tracer a("hello");
    std::cout << "-- 仅 std::move(a)，不接收结果 --\n";
    static_cast<void>(std::move(a));   // 强转 void 压掉 nodiscard 警告
    std::cout << "a.data = " << a.data << "（原封不动）\n";

    std::cout << "-- 用 move 的结果去构造 --\n";
    Tracer b = std::move(a);
    std::cout << "a.data = " << a.data << "（被掏空）\n";

    std::cout << "-- const 对象 move --\n";
    const Tracer c("const data");
    Tracer d = std::move(c);
    std::cout << "c.data = " << c.data << "（还在！）\n";
}
```

实测输出：

```
-- 仅 std::move(a)，不接收结果 --
a.data = hello（原封不动）
-- 用 move 的结果去构造 --
移动构造
a.data = （被掏空）
-- const 对象 move --
拷贝构造
c.data = const data（还在！）
```

---

## 常见错误（新手踩坑）

1. **以为 `std::move` 会移动**。它只是 cast；单独写 `std::move(x);` 什么都不做（libc++ 实测还会吃 `nodiscard` 警告）。移动发生在用转换结果去构造/赋值的那一刻。
2. **对万能引用用 `std::move`**。`template<class T> void f(T&& x) { g(std::move(x)); }`——传左值时左值被掏空。修正：`g(std::forward<T>(x));`（详见 [Item 25](item25-move-vs-forward-usage.md)）。
3. **`return std::move(local)`**。阻止 NRVO，把"零开销"变成"一次移动"。直接 `return local;`（实测见 [Item 25](item25-move-vs-forward-usage.md) 的构造次数对比）。
4. **move 后还读原对象**。被移动的对象处于"有效但未指定"状态——可以析构、可以重新赋值，不能假设内容。`std::string` 实测变空是实现细节，不是标准保证。

---

## HFT 关联

- **零开销认知**：`move`/`forward` 是编译期 cast，不生成指令。热路径里放心用，成本全在"移动构造本身做了什么"。
- **订单簿扩容**：`vector<Order>` 重新分配时移动构造把每个元素 O(1) 交接（前提：`noexcept`，见 [Item 29](item29-move-not-exist.md) 与 [Item 14](../ch03-moving-to-modern-cpp/item14-noexcept.md)）。
- **const 是移动的敌人**：配置结构体一旦 `const`，所有"move"都退化为拷贝。热路径上需要转移的对象别加 const。
- **回调转发**：`template<class F> void onEvent(F&& cb) { store(std::forward<F>(cb)); }` 保留值类别，避免拷贝大 lambda 捕获包。

---

## 自测题

**1. `std::move` 实际做了什么？它本身移动资源吗？**

<details><summary>答案</summary>
它就是一次 `static_cast<T&&>`——无条件把实参转成右值，自身不移动任何东西、不生成代码。实测：单独 `std::move(a);` 后 `a.data` 原封不动。真正的移动发生在用转换结果去构造/赋值时的重载决议。
</details>

**2. `std::move` 和 `std::forward` 的区别？**

<details><summary>答案</summary>
`move` 无条件转右值；`forward<T>` 有条件——`T` 是引用类型（实参为左值）时保持左值，`T` 非引用（实参为右值）时才转右值。实测：传左值给万能引用，`forward` 版本走左值重载，`move` 版本错误地走右值重载。口诀：右值引用配 move，万能引用配 forward。
</details>

**3. 对 const 对象 `std::move` 会发生什么？为什么？**

<details><summary>答案</summary>
静默退化为拷贝。`const T` 被 move 后类型是 `const T&&`，绑定不到移动构造的 `T&&`（移动要修改源对象，const 禁止），重载决议选中 `const T&` 拷贝构造。实测输出"拷贝构造"且源对象数据完好。不报错不警告。
</details>

**4. 为什么说 `std::forward` 是"条件 move"？条件是什么？**

<details><summary>答案</summary>
条件是模板参数 `T` 的推导结果：万能引用接左值时 `T = U&`，`forward<U&>` 不转换；接右值时 `T = U`，`forward<U>` 等价于 `move`。`T` 携带了实参原始的左右值信息，这就是 `forward` 必须显式写 `<T>` 的原因。
</details>

**5. 被 move 后的对象处于什么状态？能做什么、不能做什么？**

<details><summary>答案</summary>
标准保证"有效但未指定"：可以析构、可以赋新值、可以调无前置条件的成员（如 `clear()`），不能假设内容（比如不能假设 string 变空——虽然常见实现确实置空）。
</details>

**6. 如果类型根本没有移动构造，`std::move` 会怎样？**

<details><summary>答案</summary>
静默退回拷贝：`const T&` 拷贝构造也能绑定右值，重载决议选中它。不报错不警告——这正是 Item 29"假定移动操作不存在"的出发点。
</details>

**7. 下面代码有什么问题？**

```cpp
template<class T>
void process(T&& x) { store(std::move(x)); }
std::string s = "data";
process(s);
```

<details><summary>答案</summary>
`x` 是万能引用，传左值 `s` 时 `T = std::string&`，`std::move(x)` 无条件转右值，`store` 走移动重载把 `s` 掏空。调用方"只是传了个参数"数据就没了。修正：`store(std::forward<T>(x));`
</details>

---

## 参考与延伸

- 下一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)
- 回到：[第 5 章 右值引用、移动语义与完美转发](README.md)
