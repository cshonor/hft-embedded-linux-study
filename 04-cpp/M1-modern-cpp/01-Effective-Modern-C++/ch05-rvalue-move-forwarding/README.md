# 第 5 章 右值引用、移动语义与完美转发

**Rvalue References, Move Semantics, and Perfect Forwarding** — Items 23–30

## 本章讲什么

移动语义让"资源搬运"代替"资源拷贝"成为可能——`vector` 扩容、`string` 返回、容器元素转移都能从 O(n) 拷贝降到 O(1) 指针交接。完美转发让泛型包装能原样转发实参的左右值性与 const 性。但这两个机制交织着万能引用、引用折叠、`std::move`/`std::forward` 的精确语义，是 Modern C++ 最易踩坑的一章。

---

## 各 Item 要点

### Item 23：理解 `std::move` 和 `std::forward`

**`std::move` 不移动任何东西**——它是一个无条件的右值转换（`static_cast<T&&>`）。真正移动发生在移动构造/赋值函数里。`std::forward` 是**有条件**的右值转换：仅当原始实参是右值时才转成右值。

```cpp
std::move(x);   // x 无条件变右值
std::forward<T>(x);  // 仅当 T 是非引用（右值）时才转右值
```

**关键**：`move` 用于"我要无条件搬走它"的场景；`forward` 用于"我要保留它的左右值性转发"的场景。混用会导致意外拷贝或意外移动。

### Item 24：区分万能引用和右值引用

`T&&` 在**类型推导发生的语境**（模板 + `auto`）里是万能引用，能绑左值也能绑右值；在无推导的语境里是纯右值引用，只绑右值。

```cpp
void f(Widget&& w);               // 右值引用（无推导）
template<class T> void g(T&& x);  // 万能引用（有推导）
auto&& x = expr;                  // 万能引用
```

**万能引用的推导**：左值实参 → `T` 推为 `T&`（引用折叠）；右值实参 → `T` 推为 `T`。这就是"万能"的根源。

### Item 25：对万能引用使用 `std::forward`，对右值引用使用 `std::move`

```cpp
template<class T>
void set(T&& x) { target(std::forward<T>(x)); }  // 万能引用 → forward

void take(Widget&& w) { target(std::move(w)); }  // 右值引用 → move
```

**致命错误**：对万能引用用 `std::move`——若实参是左值，`move` 会无条件搬走它，调用方的左值被掏空，悬垂。`forward` 才是保留语义的正确选择。

### Item 26：避免对万能引用重载

万能引用重载会"贪婪匹配"——几乎任何实参都最优匹配万能引用版本，导致其他重载被遮蔽：

```cpp
class Person {
public:
    template<class T> Person(T&& n);   // 贪婪：连 Person 本身、int 都匹配
    Person(int idx);                   // 被遮蔽
};
```

拷贝构造也会被万能引用构造"劫持"——`Person p2(p1)`（`p1` 是 `Person&`）会匹配万能引用版本而非拷贝构造，在 `n` 是 `Person` 时可能错误地转发给 `string` 构造，编译失败或行为异常。

### Item 27：万能引用重载的替代方案

- **标签分发（tag dispatch）**：用一个 `template<class T> void log(T&&)` 转发到 `log_impl(T&&, std::true_type/false_type)` 重载，按条件分派，避免重载歧义。
- **`enable_if` 约束模板**：用 `std::enable_if_t<condition>` 在万能引用模板上施加"仅当 T 不是 Person 本身才启用"的约束，把 Person 的拷贝/构造让给普通构造函数。C++14 用 `std::enable_if`，C++20 用 Concepts 更干净。
- **放弃万能引用重载**：直接用具名参数重载（多个 `set(const string&)` / `set(int)`）。

### Item 28：理解引用折叠

引用折叠是万能引用与完美转发的底层机制。四条规则：`&` + `&` → `&`；`&` + `&&` → `&`；`&&` + `&` → `&`；`&&` + `&&` → `&&`。**只要有左值引用参与，折叠结果是左值引用**。

折叠发生在四个语境：模板推导、`auto` 推导、`typedef`/`using`、`decltype`。它解释了"万能引用接左值为什么 `T` 变 `T&`"。

### Item 29：认识移动操作不存在或廉价的情形

移动不是万能的：
- **没有移动构造**：旧类、C 兼容结构体没有移动操作，`std::move` 退回拷贝。
- **移动不比拷贝快**：`array<T, N>` 的移动是逐元素移动（O(N)）；小类型（`int`、指针）移动 = 拷贝。
- **常量对象不能移动**：`const T&&` 的移动构造无法修改对象，退回拷贝。`const shared_ptr` 拷贝而非移动。

**结论**：不要无脑 `std::move` 返回值（RVO/NRVO 已经做更好）；不要 `move` 局部 `const` 对象；对容器 `vector<vector<T>>` 的移动才是 O(1) 真收益。

### Item 30：熟悉完美转发失败的处境

完美转发在以下场景失败：
1. **大括号初始化**：`{1,2,3}` 无法转发（模板不推导 braced-init-list）。要转成 `initializer_list` 显式传递。
2. **0 或 NULL 当空指针**：推导为 `int` 而非指针。
3. **重载的函数指针 / 位字段**：无法取地址 / 无法绑定非 const 引用到位字段。

---

## HFT 关联

- **移动语义与订单簿扩容**：`vector<Order>` 扩容时，`Order` 的移动构造（O(1) 指针交接）让扩容从 O(N) 拷贝降到 O(N) 移动——前提是 `Order` 有 `noexcept` 移动构造（见 ch03 Item 14），否则 STL 退回拷贝。这是 HFT C++ 性能的隐形开关。
- **`std::move` 误用悬垂**：热路径里 `move` 一个还在用的对象是悬垂引用的直接来源。规则——"最后使用处才 `move`，且之后不再访问"。
- **完美转发与回调注册**：muduo 的 `template<class F> void setCallback(F&& f)` 用万能引用 + `forward` 把 lambda / 函数指针 / 仿函数原样转发存储，避免不必要的拷贝。读这类框架代码必须懂万能引用。
- **`vector<vector<Tick>>` 移动**：回测里按 symbol 分桶的行情数据，桶间用 `move` 转移是 O(1)；但桶内 `array<Tick, N>` 的移动是 O(N)——选 `vector` 而非 `array` 才能享受移动红利。

---

## 自测题

1. `std::move` 实际做了什么？它本身会移动资源吗？
2. `void f(Widget&&)` 和 `template<class T> void g(T&&)` 的 `&&` 有何不同？什么叫万能引用？
3. 对万能引用形参用 `std::move` 为什么危险？应该用什么？
4. 引用折叠的四条规则是什么？"有左值参与就折叠为左值"如何解释万能引用接左值时 `T` 变 `T&`？
5. `std::vector<std::array<int, 1000>>` 的移动是 O(1) 还是 O(N)？为什么无脑 `move` 不总是有效？



## 代码自测

### Q1: std::move 不移动

```cpp
std::string s = "hello";
auto len = std::move(s).length();
// s 还能正常使用吗？
```

> `std::move(s).length()` 后 `s` 还有效吗？

<details>
<summary>答案与复习指引</summary>

**`s` 仍然有效。** `std::move` 只是 `static_cast<string&&>(s)`——它不移动任何东西。`.length()` 是 const 方法不修改对象。`s` 的内容仍是 `"hello"`。

**`std::move` 真正起作用的场景：** `string r = std::move(s);`——移动构造函数被调用，`s` 的内部指针转移给 `r`，`s` 变为空。

**教训：** `std::move` 本身零开销零副作用，它只是"请求"移动；真正移动发生在移动构造/赋值函数里。

**复习：** → [Item 23：理解 std::move 和 std::forward](./item23-理解std-move和std-forward.md)
</details>

### Q2: 万能引用 vs 右值引用

```cpp
void f(std::string&& s);               // A: 右值引用还是万能引用？
template<typename T>
void g(T&& x);                          // B: 右值引用还是万能引用？
auto&& y = expr;                        // C: 右值引用还是万能引用？
```

> A、B、C 分别是右值引用还是万能引用？万能引用能接受左值吗？

<details>
<summary>答案与复习指引</summary>

- A: **右值引用**（无类型推导，只绑右值）
- B: **万能引用**（有模板类型推导，能绑左值和右值）
- C: **万能引用**（有 auto 推导）

**万能引用接左值时：** `T` 推导为 `T&`（引用折叠 `T& &&` → `T&`），`x` 成为左值引用。接右值时 `T` 推导为 `T`，`x` 是右值引用。

**关键判别：** `T&&` 在有类型推导的语境（模板/auto）里是万能引用；在无推导的语境（如 `void f(string&&)`）是纯右值引用。

**复习：** → [Item 24：区分万能引用和右值引用](./item24-区分万能引用和右值引用.md)
</details>

### Q3: 对万能引用用 move 的灾难

```cpp
template<typename T>
void set(T&& x) {
    target(std::move(x));  // 危险！
}
std::string s = "hello";
set(s);  // s 会被掏空吗？
```

> `set(s)` 后 `s` 会怎样？应该用什么替代？

<details>
<summary>答案与复习指引</summary>

**`s` 被掏空（悬垂）。** `s` 是左值，`T` 推导为 `string&`，`x` 是 `string&`（左值引用）。但 `std::move(x)` 无条件转成右值——`target` 的移动构造被调用，`s` 的内容被搬走。

**正确做法：** `target(std::forward<T>(x));`——`forward` 是有条件的右值转换：仅当原始实参是右值时才转右值，是左值则保持左值。

**规则：** 对万能引用用 `forward`，对右值引用用 `move`。永不混用。

**复习：** → [Item 25：对万能引用使用 std::forward](./item25-对万能引用使用std-forward对右值引用使用std-move.md)
</details>
