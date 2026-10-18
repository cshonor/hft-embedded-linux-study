# Item 41：可拷贝参数考虑按值传递

> 第 8 章 微调 · Item 41 · 上一节：[Item 40 atomic vs volatile](../ch07-concurrency-api/item40-atomic-vs-volatile.md) · 下一节：[Item 42 emplace vs insert](item42-emplace-vs-insert.md)

## 为什么要学这个（先建立直觉）

假设你要写一个函数，把名字存进容器。C++98 时代的老规矩是传 `const&`，进容器时拷贝一次：

```cpp
void addName(const std::string& name) { names.push_back(name); }   // 左值右值都拷贝
```

后来学了移动语义，你发现传右值时那次拷贝是冤枉的，于是加成两个重载——一个接左值、一个接右值，各用最省的构造方式。代码对了，但每个这样的函数都要写两遍。

这一节给出第三种写法：**按值传参，进容器时 `std::move`**。一个函数替代两个重载，左值来就"拷贝 + 移动"，右值来就"移动 + 移动"，每种情况只比最优方案多付一次廉价的移动。这是 C++11 之后社区里广泛流行的惯用法（`std::string`、容器、智能指针的参数经常这么写），但它的适用边界非常清晰——搞不清边界，省下的代码行会加倍在性能上还回去。

---

## 这节讲什么

按值传参 + `std::move` 进容器，用**一个函数**替代 `const&` + `&&` 两个重载。代价模型：左值多付一次移动，右值多付一次移动。用带计数器的类实测四种组合的构造次数，给出三条适用前提（可移动、move 廉价、参数总会被拷一份），以及不适用场景（只读参数、多参数重载爆炸、基类切片）。

---

## 三种写法的代价模型

```cpp
// 方案 A：两个重载（性能最优，代码两份）
void add(const Widget& w) { v.push_back(w); }           // 左值：1 次拷贝
void add(Widget&& w)      { v.push_back(std::move(w)); } // 右值：1 次移动

// 方案 B：按值 + move（一份代码）
void add(Widget w) { v.push_back(std::move(w)); }
// 左值：拷贝构造形参 + 移动进容器
// 右值：移动构造形参 + 移动进容器

// 方案 C：万能引用 + forward（最优性能，模板，Item 26 的重载坑）
template<class T> void add(T&& w) { v.push_back(std::forward<T>(w)); }
```

理论代价表（书中版本）：

| 实参 | A 两个重载 | B 按值传递 | C 万能引用 |
|------|-----------|-----------|-----------|
| 左值 | 1 拷贝 | 1 拷贝 + 1 移动 | 1 拷贝 |
| 右值 | 1 移动 | 2 移动 | 1 移动 |

按值传递对每种实参都多付**恰好一次移动**——这就是"代码简洁"的价格。值不值，取决于移动的成本。

---

## 实测：构造计数器对账（本机实测 clang 23.1.0）

用带构造/拷贝/移动计数器的 `W`，两种方案分别喂左值、纯右值、具名右值：

```cpp
#include <cstdio>
#include <vector>

struct Cnt { int ctor = 0, copy = 0, move = 0; };
Cnt g;

struct W {
    W() { ++g.ctor; }
    W(const W&) { ++g.copy; }
    W(W&&) noexcept { ++g.move; }
};

void reset(const char* label) {
    std::printf("%-34s ctor=%d copy=%d move=%d\n", label, g.ctor, g.copy, g.move);
    g = Cnt{};
}

struct BoxA {                                // 方案 A：两个重载
    std::vector<W> v;
    void add(const W& w) { v.push_back(w); }
    void add(W&& w)      { v.push_back(std::move(w)); }
};

struct BoxB {                                // 方案 B：按值 + move
    std::vector<W> v;
    void add(W w) { v.push_back(std::move(w)); }
};

int main() {
    { BoxA a; W w; g = Cnt{}; a.add(w);             reset("A(双重载) add(左值):"); }
    { BoxB b; W w; g = Cnt{}; b.add(w);             reset("B(按值)   add(左值):"); }
    { BoxA a; g = Cnt{}; a.add(W{});                reset("A(双重载) add(纯右值 W{}):"); }
    { BoxB b; g = Cnt{}; b.add(W{});                reset("B(按值)   add(纯右值 W{}):"); }
    { BoxA a; W w; g = Cnt{}; a.add(std::move(w));  reset("A(双重载) add(std::move(w)):"); }
    { BoxB b; W w; g = Cnt{}; b.add(std::move(w));  reset("B(按值)   add(std::move(w)):"); }
    return 0;
}
```

clang 23.1.0 + `-std=c++17 -Wall` 实测输出：

```
A(双重载) add(左值):              ctor=0 copy=1 move=0
B(按值)   add(左值):              ctor=0 copy=1 move=1
A(双重载) add(纯右值 W{}):        ctor=1 copy=0 move=1
B(按值)   add(纯右值 W{}):        ctor=1 copy=0 move=1
A(双重载) add(std::move(w)):      ctor=0 copy=0 move=1
B(按值)   add(std::move(w)):      ctor=0 copy=0 move=2
```

对账结果有两个值得注意的点：

1. **左值**完全符合理论：B 比 A 多一次移动（copy=1, move=1 vs copy=1, move=0）。
2. **纯右值 `W{}` 两方案打平**（都是 ctor=1, move=1）！因为 C++17 起强制拷贝消除，纯右值直接原地构造进按值形参，省了书中预测的一次移动。**具名右值 `std::move(w)`** 不能消除，B 才显出 2 次移动 vs A 的 1 次。

也就是说：在现代标准 + 现代编译器上，按值传参的"右值税"比书里写的还轻——只有传**具名**右值时才真的多付一次移动。

---

## 适用前提与反模式

按值传参划算，需要三条同时成立：

1. **类型可移动且移动廉价**：`std::string`、`std::vector`、智能指针这类指针交接 O(1)；`std::array<T, N>` 的移动是逐元素拷贝 O(N)，多付的那次"移动"等于又一次拷贝，不划算。
2. **参数总会被拷一份存起来**：`add` 把参数放进容器/成员，拷贝必然发生。如果函数只是**读**参数（`double norm(const Vec&)`），按值传参是纯浪费。
3. **拷贝与移动成本同量级**：形参构造那一下躲不掉，若拷贝远贵于移动，左值路径依然心疼。

两个结构性限制：

- **多参数组合爆炸**：两个这样的参数用重载方案要 4 个函数（2²），n 个参数要 2ⁿ 个——这正是按值传参最大的现实动机；反过来，如果参数**不一定**被存（某分支提前 return），按值传参就白付了形参构造费。
- **按值接基类会切片**：`void f(Base b)` 收到 `Derived` 时切掉派生部分。实测（同一程序扩展）：

```cpp
struct Base { virtual int kind() const { return 1; } };
struct Derived : Base { int kind() const override { return 2; } };
// by_value(d) = 1   by_ref(d) = 2    ← 按值接到了切剩的 Base
```

多态类型参数永远用引用/指针，不按值。

---

## 完整可运行例子

上方"实测"一节即完整可运行程序（clang 23.1.0，`-std=c++17 -Wall`），输出已给出；切片验证代码见"适用前提"小节末尾。

---

## 常见错误（新手踩坑）

**错误 1：按值接收后忘了 `std::move` 进容器**

```cpp
void add(std::string name) {
    names.push_back(name);          // name 是具名左值 → 拷贝！按值的收益全没了
}
```

**修正：** `names.push_back(std::move(name));`——形参是左值，必须显式 move。

**错误 2：移动昂贵的类型按值传递**

```cpp
void add(std::array<double, 1024> a) { buf.push_back(std::move(a)); }  // 移动 = O(N) 拷贝
```

**修正：** `std::array`、大 POD 聚合体用 `const&`（只读）或双重载。

**错误 3：只读参数也按值传**

```cpp
void print_name(std::string name) { std::cout << name; }   // 白拷/白移一次
```

**修正：** 只读参数用 `const std::string&`。

**错误 4：参数可能不存时按值传**

```cpp
void maybe_add(std::string s) { if (!validate(s)) return; store(std::move(s)); }
// validate 失败的路径白付了构造形参的钱
```

**修正：** 有条件存储的用 `const&` 接收，确定要存时再拷。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 传参 | 按值（拷贝）或指针 | 按值 / `const&` / `&&` / 万能引用 | 多了值类别维度 |
| "省一次拷贝" | 不存在（没有移动语义） | 右值走移动构造 | C++11 |
| 惯用法 | 大结构体传指针 | 总会存的参数可按值 + move | 一份代码 ≈ 双重载性能 |

**一句话总结：** 参数**总会被拷一份**、类型**移动廉价**时，按值传参 + `std::move` 用一份代码拿到接近双重载的性能；否则乖乖用引用。

---

## HFT 关联

- **配置/注册接口**：`registerSymbol(std::string sym)` 这类冷路径接口按值传参最舒服——移动廉价、必然存进 map，一份代码够简洁。
- **热路径不这么玩**：`on_tick(Tick t)` 若 `Tick` 含 `std::array` 或大 buffer，按值形参构造就是 O(N) 拷贝，热路径用 `const&`。
- **具名右值税**：实测确认 `f(std::move(w))` 时按值方案付 2 次移动——对象若装着堆内存池句柄，这两次移动里的指针交接虽小，在高频调用下也要计入延迟预算。
- **多参数构造函数**：订单构建器这类 4-5 个 string 参数的函数，重载方案 32 个函数不现实，按值传参是唯一可维护的选择。

---

## 自测题

**1. 按值传参相比 `const&`+`&&` 双重载，对左值实参多付了什么代价？**

<details><summary>答案</summary>
恰好一次移动构造：双重载版左值是 1 次拷贝；按值版是"拷贝构造形参 + 移动进容器"共 1 拷贝 + 1 移动。实测 `A(双重载) add(左值): copy=1 move=0` vs `B(按值) add(左值): copy=1 move=1`。
</details>

**2. 实测中传纯右值 `W{}` 时两种方案为什么打平？**

<details><summary>答案</summary>
C++17 起强制拷贝消除：纯右值直接原地构造进按值形参，省掉书中预测的"移动构造形参"那一步，两方案都是 ctor=1, move=1。只有传具名右值（`std::move(w)`）时才显出按值版多一次移动（move=2 vs 1）。
</details>

**3. 按值传参惯用法的三条适用前提是什么？**

<details><summary>答案</summary>
①类型可移动且移动廉价（string/vector/unique_ptr，而非 std::array）；②参数总会被拷一份存入内部（不是只读、不是条件存储）；③拷贝与移动成本同量级。三条同时成立时，一份代码换接近双重载的性能是划算交易。
</details>

**4. 为什么 `std::array<T, N>` 不适合按值传参？**

<details><summary>答案</summary>
它的移动是逐成员拷贝 O(N)，按值方案"多付一次移动"对它等于多付一次完整拷贝，代价模型失效。这类类型用 `const&`（只读）或双重载。
</details>

**5. 按值接收基类参数会发生什么？实测结果是什么？**

<details><summary>答案</summary>
切片：派生对象被切成基类副本，虚函数行为丢失。实测 `by_value(d)=1`（Base 的 kind）vs `by_ref(d)=2`（Derived 的 kind）。多态类型参数必须用引用或指针。
</details>

**6. 两个按值存储的参数用重载方案要写几个函数？这说明了什么？**

<details><summary>答案</summary>
2² = 4 个，n 个参数要 2ⁿ 个。重载方案在多参数下组合爆炸，这是按值传参最大的现实动机——可维护性。反过来参数"可能不存"时，按值会在放弃的分支里白付构造费。
</details>

**7. `void add(std::string name) { names.push_back(name); }` 错在哪？**

<details><summary>答案</summary>
`name` 是具名形参，是左值——`push_back(name)` 走拷贝，左值实参总代价变成 1 拷贝 + 1 拷贝，按值的收益被完全抵消。必须 `push_back(std::move(name))`。
</details>

---

## 参考与延伸

- 上一节：[Item 40 atomic vs volatile](../ch07-concurrency-api/item40-atomic-vs-volatile.md)
- 下一节：[Item 42 emplace vs insert](item42-emplace-vs-insert.md)
- 回到：[第 8 章 微调](README.md)
