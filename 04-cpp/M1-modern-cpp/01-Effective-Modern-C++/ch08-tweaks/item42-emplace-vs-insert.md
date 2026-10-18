# Item 42：优先 emplace 而非 insert

> 第 8 章 微调 · Item 42 · 上一节：[Item 41 按值传递](item41-pass-by-value.md)

## 为什么要学这个（先建立直觉）

往 `vector` 里加一个 `Widget`，两种写法看着差不多：

```cpp
v.push_back(Widget(42, 100));   // 先在栈上造一个临时 Widget，再移动进容器
v.emplace_back(42, 100);        // 把构造参数送进容器，在容器的内存里原地造
```

第一行其实干了三件事：构造临时对象、移动进容器、析构临时对象。第二行只干一件事：拿参数在容器尾部那块还没初始化的内存上 placement-new 一个对象。**省掉的是整个临时对象的一生**——构造、移动、析构。

但 `emplace` 不是无脑替换 `push_back` 的银弹。它用的是**直接初始化**语义，这意味着两件事：一是它能绕过 `explicit`（有时候是便利，有时候是踩进编译器都不拦你的坑）；二是参数原样转发给构造函数，传错参数时编译器给你的报错和 `push_back` 完全不同。再加上智能指针容器那个著名的异常泄漏陷阱，这一节的正确姿势是"优先 emplace，但知道何时不该用"。

---

## 这节讲什么

`emplace_back(args...)` 在容器内**原地构造**元素，省去临时对象的构造 + 移动 + 析构。实测对比两者构造次数；分析三类 emplace 反而更差/更危险的场景：实参已是元素类型、`explicit` 构造函数被直接初始化绕过（`vector<regex>` 实测编译差异）、智能指针容器传裸 `new` 的异常泄漏。

---

## 实测 1：构造次数对账（本机实测 clang 23.1.0）

带构造/拷贝/移动/析构计数器的 `W`，对比三种插入方式：

```cpp
#include <cstdio>
#include <vector>

struct Cnt { int ctor = 0, copy = 0, move = 0, dtor = 0; };
Cnt g;

struct W {
    W(int x, int y) { ++g.ctor; (void)x; (void)y; }
    W(const W&) { ++g.copy; }
    W(W&&) noexcept { ++g.move; }
    ~W() { ++g.dtor; }
};

void report(const char* label) {
    std::printf("%-40s ctor=%d copy=%d move=%d dtor=%d\n",
                label, g.ctor, g.copy, g.move, g.dtor);
    g = Cnt{};
}

int main() {
    {
        std::vector<W> v;
        v.reserve(1);                 // 排除扩容干扰
        g = Cnt{};
        v.push_back(W(42, 100));      // 临时对象 + 移动
        report("push_back(W(42,100)):");
    }
    {
        std::vector<W> v;
        v.reserve(1);
        g = Cnt{};
        v.emplace_back(42, 100);      // 原地构造
        report("emplace_back(42,100):");
    }
    {
        std::vector<W> v;
        v.reserve(1);
        W w(42, 100);
        g = Cnt{};
        v.push_back(w);               // 传左值：两者都要拷贝
        report("push_back(w) 传左值:");
        v.pop_back();                 // 析构掉刚插入的元素（dtor 计数 +1）
        v.emplace_back(w);
        report("emplace_back(w) 传左值:");
    }
    return 0;
}
```

clang 23.1.0 + `-std=c++17 -Wall` 实测输出：

```
push_back(W(42,100)):                    ctor=1 copy=0 move=1 dtor=1
emplace_back(42,100):                    ctor=1 copy=0 move=0 dtor=0
push_back(w) 传左值:                  ctor=0 copy=1 move=0 dtor=0
emplace_back(w) 传左值:               ctor=0 copy=1 move=0 dtor=1
```

逐行解读：

1. **临时对象场景**：`push_back(W(42,100))` 是 ctor=1 + move=1 + dtor=1（临时对象完整的一生）；`emplace_back(42,100)` 只有 ctor=1。**emplace 精确省掉一次移动和一次析构**——对 `std::string` 这种移动要交接三个指针的类型，省的是几十条指令。
2. **左值场景**：两者打平（各 copy=1）。最后一行的 `dtor=1` 来自中间那次 `pop_back()`（把上一轮插入的元素析构了），不是 emplace 的开销——**实参已经是元素类型时，emplace 没有任何收益**，它和 push_back 一样只是转发给拷贝/移动构造。

结论和 [Item 41](item41-pass-by-value.md) 的教训一致：emplace 的收益只在"用构造参数原地造"时存在，手里已经有现成对象时两者等价。

---

## 实测 2：explicit 陷阱——emplace 能编译，push_back 不能

`std::regex` 的 `const char*` 构造函数是 `explicit`。同一个 `vector<regex>`，两种插入方式编译结果不同：

```cpp
#include <regex>
#include <vector>

int main() {
    std::vector<std::regex> v;
    v.emplace_back(".*");   // 编译通过
    return 0;
}
```

clang 23.1.0 实测：**编译通过**。

```cpp
    v.push_back(".*");      // 换成 push_back
```

clang 23.1.0 实测报错原文：

```
error: no matching member function for call to 'push_back'
    v.push_back(".*");
    ~~^~~~~~~~~
note: candidate function not viable: no known conversion from
      'const char[3]' to 'const std::regex' for 1st argument
note: candidate function not viable: no known conversion from
      'const char[3]' to 'value_type' (aka 'std::regex') for 1st argument
```

原因：`push_back` 的形参是 `const regex&` / `regex&&`，传 `".*"` 需要一次 `const char* → regex` 的**隐式转换**（拷贝初始化），`explicit` 禁止它；`emplace_back` 把参数**直接转发给构造函数**（直接初始化），`explicit` 构造函数名正言顺地被调用。

这是双刃剑。便利的一面：`emplace_back` 能搞定 push_back 表达不了的插入。危险的一面：**拷贝初始化是你和编译器之间的默契检查**——隐式转换走不通时 push_back 直接编译失败，而 emplace 会默默地直接构造。经典事故：

```cpp
std::vector<std::string> vs;
vs.emplace_back(nullptr);   // 能编译！用 nullptr 构造 string → 运行时 UB/崩溃
vs.push_back(nullptr);      // 编译失败——反而安全
```

`explicit` 的设计意图是"这种转换必须显式写出"，emplace 的直接初始化把这道护栏拆掉了。参数和元素类型不完全吻合时，想清楚再 emplace。

---

## 智能指针容器：异常安全的分水岭

```cpp
std::vector<std::unique_ptr<Widget>> v;
v.emplace_back(new Widget);                 // 能编译，但有泄漏窗口
v.push_back(std::make_unique<Widget>());    // 安全
```

第一行的问题在求值顺序：`new Widget` **先于** `emplace_back` 执行，得到一个裸指针；emplace 内部若因扩容分配抛 `std::bad_alloc`，这个裸指针还没被任何 `unique_ptr` 接管——**内存泄漏**。

第二行里 `make_unique` 在 `push_back` 之前就完成了 RAII 接管，即使扩容抛异常，临时 `unique_ptr` 析构也会把对象收走。规则一句话：**往智能指针容器里插新对象，用 `push_back(make_unique<T>())`**；emplace 的参数应该是"构造参数"，不该是裸 `new` 表达式。

---

## 决策表

| 场景 | 推荐 | 理由 |
|------|------|------|
| 用构造参数新造元素 | `emplace_back(args...)` | 原地构造，省临时对象一生（实测省 1 move + 1 dtor） |
| 手里已有元素类型对象 | 随意，两者等价 | 实测左值场景 copy=1 打平 |
| 智能指针容器装新对象 | `push_back(make_unique<T>())` | RAII 先接管，无异常泄漏窗口 |
| 参数需隐式转换才匹配 | 优先 `push_back` | 拷贝初始化帮你挡住 explicit 误用 |
| 性能极度敏感且类型平凡可拷 | 实测后再定 | emplace 也可能因完美转发模板代码膨胀 |

---

## 完整可运行例子

"实测 1"即完整可运行程序（clang 23.1.0，`-std=c++17 -Wall`），输出已给出；"实测 2"两个 8 行文件分别演示编译通过与编译失败。

---

## 常见错误（新手踩坑）

**错误 1：emplace 传已构造好的对象**

```cpp
v.emplace_back(Widget(42, 100));   // 还是构造了临时对象，和 push_back 一样
```

**修正：** `v.emplace_back(42, 100);`——传构造参数，不是传对象。

**错误 2：智能指针容器 `emplace_back(new T)`**

```cpp
v.emplace_back(new Widget);   // 扩容抛异常 → 裸指针泄漏
```

**修正：** `v.push_back(std::make_unique<Widget>());`

**错误 3：忽略 emplace 绕过 explicit 的杀伤力**

```cpp
vs.emplace_back(nullptr);   // 能编译，运行时炸
```

**修正：** 参数不是元素类型时，警惕 emplace 的直接初始化语义；`push_back` 的编译错误有时是救命的。

**错误 4：以为 emplace 一定更快**

**修正：** 实参已是元素类型时两者等价（实测打平）；容器已持有对象、要做的是"插入现有对象"时，insert/push_back 反而更直白。收益只在原地构造场景。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 数组追加 | `arr[n++] = tmp`（先造再拷） | `push_back` / `emplace_back` | 后者原地构造 |
| 构造与放置 | 分离（先声明再赋值） | placement new 一体完成 | 容器内存管理 |
| 异常安全 | 手动 | RAII（make_unique 先接管） | 栈展开自动清理 |

**一句话总结：** 有构造参数就 `emplace_back(args...)` 原地构造；手里已是元素对象两者等价；智能指针容器永远 `push_back(make_unique<T>())`。

---

## HFT 关联

- **行情对象入队**：`ticks.emplace_back(symbol, price, qty, ts)` 原地构造省一次 Tick 的移动——Tick 若含 `std::string` symbol，高频下收益可观。
- **预分配配合**：emplace 省构造、`reserve` 省扩容，两者搭配才是完整优化（实测里特意 `reserve(1)` 排除扩容干扰）。
- **订单簿 map**：`book.emplace(price, Order{...})` 避免临时 `pair`；但 key 已存在时 emplace 会先构造再丢弃——查插混合场景用 `try_emplace`（C++17）。
- **连接池/对象池**：`vector<unique_ptr<Session>>` 只准 `push_back(make_shared/make_unique)`——热路径上一次 bad_alloc 泄漏不会被立刻发现，但会慢慢吃掉内存。

---

## 自测题

**1. `emplace_back(42, 100)` 相比 `push_back(W(42, 100))` 省掉了什么？实测数据？**

<details><summary>答案</summary>
省掉临时对象的移动构造和析构。实测：push_back 是 ctor=1, move=1, dtor=1；emplace_back 只有 ctor=1。emplace 把参数完美转发给构造函数，在容器尾部未初始化内存上 placement-new，不产生临时对象。
</details>

**2. 传左值（已构造好的对象）时两者有差别吗？**

<details><summary>答案</summary>
没有。实测两者都是 copy=1, move=0。emplace 此时只是转发给拷贝构造，和 push_back 等价。emplace 的收益只在"用构造参数原地造"时存在。
</details>

**3. `vector<regex>` 上 `emplace_back(".*")` 和 `push_back(".*")` 的编译结果分别是什么？为什么？**

<details><summary>答案</summary>
前者编译通过，后者报错 `no known conversion from 'const char[3]' to 'const std::regex'`。regex 的 const char* 构造函数是 explicit：push_back 需要隐式转换（拷贝初始化）被 explicit 拒绝；emplace 用直接初始化，explicit 构造函数可被直接调用。explicit 护栏对 emplace 无效。
</details>

**4. `v.emplace_back(new Widget)`（v 是 `vector<unique_ptr<Widget>>`）有什么问题？正确写法？**

<details><summary>答案</summary>
`new Widget` 先于 emplace 求值得到裸指针；若 emplace 内部扩容分配抛异常，裸指针还没被 unique_ptr 接管，内存泄漏。正确写法 `v.push_back(std::make_unique<Widget>())`——RAII 接管发生在 push_back 之前，栈展开时自动释放。
</details>

**5. 什么场景下 `push_back` 反而比 `emplace_back` 安全？**

<details><summary>答案</summary>
两类：①参数需要隐式转换才匹配元素类型时，push_back 的拷贝初始化会被 explicit 拦住（编译失败 = 安全），emplace 的直接初始化会默默构造（如 `vector<string>::emplace_back(nullptr)` 编译通过但运行时 UB）；②智能指针容器装新对象时 push_back(make_unique) 无异常泄漏窗口。
</details>

**6. map 容器上 emplace 家族有什么额外注意事项？**

<details><summary>答案</summary>
`map::emplace(k, v)` 在 k 已存在时仍会先构造好 value 再丢弃（白付构造费）；C++17 的 `try_emplace` 只在 key 不存在时才构造，查插混合场景（订单簿按价位插入）应优先 try_emplace。
</details>

**7. 为什么说"emplace 一定更快"是误解？**

<details><summary>答案</summary>
实参已是元素类型时两者实测打平（各 copy=1）；emplace 的模板完美转发会实例化更多代码（代码膨胀）；某些场景 insert 迭代器版本还能批量插入。收益精确地只来自"原地构造"这一种场景，其它场景按可读性选。
</details>

---

## 参考与延伸

- 上一节：[Item 41 按值传递](item41-pass-by-value.md)
- 回到：[第 8 章 微调](README.md)
