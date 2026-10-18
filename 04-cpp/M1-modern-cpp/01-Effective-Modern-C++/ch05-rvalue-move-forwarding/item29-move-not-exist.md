# Item 29：认识移动操作不存在或廉价的情形

> 第 5 章 右值引用、移动语义与完美转发 · Item 29 · 上一节：[Item 28 引用折叠](item28-reference-collapsing.md)

## 为什么要学这个（先建立直觉）

C 程序员的世界里，`memcpy` 对所有类型代价相同——按字节拷贝，没有"移动"的概念。学了移动语义之后，很容易走向另一个极端：以为 `std::move` 是免费的性能加速器，见对象就 move。

真相有三层，一层比一层扎心：

1. **有些类型根本没有移动操作**——`std::move` 静默退化成拷贝，不报错不警告；
2. **有些类型的移动不比拷贝快**——`std::array` 内联存储，移动是逐元素 O(N)；
3. **有些场景移动根本不会被使用**——const 对象不能移动，`vector` 扩容时缺 `noexcept` 的移动构造直接被弃用。

工程态度一句话：**假定移动操作不存在、不廉价、不被用**，除非你确认过类型和场景。

---

## 这节讲什么

移动语义失效的三种情形实测：无移动构造（静默拷贝）、移动不廉价（`std::array` 逐元素）、移动不被用（const 成员、`vector` 扩容缺 `noexcept` 走拷贝）。

---

## 情形一：移动不存在——静默变拷贝

C++98 时代的遗留类只要用户声明了拷贝构造（或析构），移动构造就**不会被生成**（Item 17 的规则）。对它 `std::move` 不会报错——`const T&` 拷贝构造也能绑右值，静默走拷贝。本机实测（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
struct Legacy {
    std::string s;
    Legacy(std::string v) : s(std::move(v)) {}
    Legacy(const Legacy& o) : s(o.s) { std::cout << "  Legacy 拷贝构造\n"; }
    // 用户声明了拷贝构造 → 移动构造不被生成
};

Legacy l1("legacy-data");
Legacy l2 = std::move(l1);      // 不报错！静默拷贝
```

实测输出：

```
== 遗留类 move 静默变 copy ==
  Legacy 拷贝构造           ← move 了个寂寞
  l1.s=legacy-data（源还在！）
```

这就是最危险的地方：**编译通过、没有警告、功能正确，只是慢**。代码 review 时看不出任何问题。

---

## 情形二：移动不廉价——std::array 逐元素

`std::vector` 的移动是 O(1)（交接三个指针），因为它把元素放在堆上。`std::array` 把元素**内联**在对象里，没有指针可交接，移动只能逐元素来。实测（接上 `Legacy`，数组里装两个）：

```cpp
std::array<Legacy, 2> arr = { Legacy("x"), Legacy("y") };
auto arr2 = std::move(arr);   // 逐元素"移动"——Legacy 没移动构造，全是拷贝
```

实测输出：

```
== std::array 的 move 是逐元素 ==
  Legacy 拷贝构造
  Legacy 拷贝构造           ← 移动一个 array，触发 N 次元素级操作
```

即使元素有移动构造，`std::array<T, N>` 的移动也是 N 次元素移动——对 `int` 这类标量，移动就是拷贝，O(N) 一次都省不了。对照：

| 类型 | 移动复杂度 | 原因 |
|------|----------|------|
| `std::vector<T>` | O(1) | 交接堆指针 |
| `std::string`（长串） | O(1) | 交接堆指针；SSO 短串退化为小拷贝 |
| `std::array<T, N>` | O(N) | 内联存储，逐元素 |
| `int` / 指针 / POD | O(1) 但 = 拷贝 | 标量没有资源可交接 |

---

## 情形三：移动不被用——const 与 noexcept

### const 成员让"移动"名存实亡

移动要掏空源对象，const 成员不能被修改——移动构造里它只能被**拷贝**。本机实测：

```cpp
struct WithConstStr {
    const std::string name;   // const string 成员
    WithConstStr(std::string n) : name(std::move(n)) {}
    WithConstStr(const WithConstStr& o) : name(o.name) { std::cout << "  拷贝构造\n"; }
    WithConstStr(WithConstStr&& o) noexcept : name(o.name) {  // 写 std::move(o.name) 也没用
        std::cout << "  \"移动\"构造（const 成员只能拷贝）\n";
    }
};

WithConstStr a("this-is-a-fairly-long-string-to-defeat-sso");
WithConstStr b = std::move(a);
```

实测输出：

```
  "移动"构造（const 成员只能拷贝）
  a.name = this-is-a-fairly-long-string-to-defeat-sso（源没有被掏空！）
```

移动构造被调用了，但 const 成员走的是拷贝——源对象的 `name` 完好无损。对 `const` 对象本身 `std::move` 更彻底：直接退化为拷贝构造（实测见 [Item 23](item23-move-and-forward.md)）。

### vector 扩容：缺 noexcept 的移动构造被弃用

`std::vector` 扩容搬移元素时有个两难的异常安全问题：如果用移动构造搬到一半抛异常，旧内存已释放、新内存里是半残元素——**强异常保证**就破了。所以标准库的策略是：移动构造 `noexcept` 才移动，否则**用拷贝**（拷贝失败旧数据还在）。本机实测：

```cpp
struct NoNoexcept {
    std::string s;
    NoNoexcept(std::string v) : s(std::move(v)) {}
    NoNoexcept(const NoNoexcept& o) : s(o.s) { std::cout << "  拷贝\n"; }
    NoNoexcept(NoNoexcept&& o) : s(std::move(o.s)) { std::cout << "  移动\n"; }  // 无 noexcept
};
// WithNoexcept 唯一区别：移动构造标了 noexcept
```

实测输出（`reserve(1)` 后 `emplace_back` 两次，第二次触发扩容搬移）：

```
== vector 扩容：移动构造无 noexcept ==
-- 触发扩容 --
  拷贝              ← 有移动构造也不用！
== vector 扩容：移动构造有 noexcept ==
-- 触发扩容 --
  移动
```

一个 `noexcept` 关键字的差别就是扩容时 O(N) 拷贝 vs O(N) 移动。这就是 [Item 14](../ch03-moving-to-modern-cpp/item14-noexcept.md) 的实战意义。

---

## 完整可运行例子

noexcept 对比实验完整代码（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <utility>

struct NoNoexcept {
    std::string s;
    NoNoexcept(std::string v) : s(std::move(v)) {}
    NoNoexcept(const NoNoexcept& o) : s(o.s) { std::cout << "  拷贝\n"; }
    NoNoexcept(NoNoexcept&& o) : s(std::move(o.s)) { std::cout << "  移动\n"; }
};

struct WithNoexcept {
    std::string s;
    WithNoexcept(std::string v) : s(std::move(v)) {}
    WithNoexcept(const WithNoexcept& o) : s(o.s) { std::cout << "  拷贝\n"; }
    WithNoexcept(WithNoexcept&& o) noexcept : s(std::move(o.s)) { std::cout << "  移动\n"; }
};

template<class T>
void growTest(const char* label) {
    std::cout << "== " << label << " ==\n";
    std::vector<T> v;
    v.reserve(1);
    v.emplace_back("one");
    std::cout << "-- 触发扩容 --\n";
    v.emplace_back("two");
}

int main() {
    growTest<NoNoexcept>("vector 扩容：移动构造无 noexcept");
    growTest<WithNoexcept>("vector 扩容：移动构造有 noexcept");
}
```

实测输出（与上文一致）：

```
== vector 扩容：移动构造无 noexcept ==
-- 触发扩容 --
  拷贝
== vector 扩容：移动构造有 noexcept ==
-- 触发扩容 --
  移动
```

---

## 常见错误（新手踩坑）

1. **以为 `std::move` 一定更快**。对无移动构造的遗留类、标量、`std::array`，move 静默变拷贝（实测不报错不警告）。先确认类型有移动构造再谈收益。
2. **移动构造忘写 `noexcept`**。`vector` 扩容直接弃用你的移动构造走拷贝（实测）。规则：移动构造和 swap 默认就该标 `noexcept`（除非真的可能抛）。
3. **给类加 `const` 成员图"安全"**。const 成员让移动构造里的该成员只能拷贝、移动赋值干脆被抑制——对象变大后这是隐性性能税。
4. **`return std::move(local)`**。阻止 NRVO，零开销变一次移动（实测见 [Item 25](item25-move-vs-forward-usage.md)）。

---

## HFT 关联

- **订单结构体必须检查移动资质**：热路径上的 `Order`/`Tick` 如果含有 `const` 成员或忘了 `noexcept` 移动构造，`vector<Order>` 每次扩容都在做全量拷贝——延迟毛刺的隐形来源。
- **固定容量选 array 要认账**：`std::array<Level, 32>` 的订单簿档位很合理（无堆分配、cache 友好），但别指望"move 一下整个簿"是 O(1)——它是 32 次逐元素搬移。要搬就用 `vector` 或交换指针。
- **遗留代码接入**：对接 C++98 时代的行情解码结构体时，默认它们没有移动语义；要么补移动构造（记得 `noexcept`），要么按拷贝做性能预算。
- **态度即默认**：写模板库（如泛型对象池）时，按"元素类型可能没有廉价移动"来设计——能用指针交换就不搬对象。

---

## 自测题

**1. 对没有移动构造的遗留类 `std::move`，会发生什么？有警告吗？**

<details><summary>答案</summary>
静默走拷贝构造（`const T&` 能绑右值），不报错不警告。实测 `Legacy` 输出"拷贝构造"且源对象数据完好。用户声明拷贝构造/析构会抑制移动构造的生成（Item 17）。
</details>

**2. `std::array<T, N>` 的移动为什么是 O(N)？**

<details><summary>答案</summary>
`array` 把元素内联存储在对象内部，没有堆指针可交接，移动构造只能逐元素移动；对标量元素移动=拷贝。实测移动 `array<Legacy, 2>` 触发 2 次元素拷贝。对比 `vector` 的移动是 O(1) 指针交接。
</details>

**3. const 成员对移动有什么影响？实测现象？**

<details><summary>答案</summary>
移动构造里 const 成员只能拷贝（不能掏空源对象的 const 部分），移动赋值则被直接抑制。实测：`WithConstStr` 的移动构造执行后，源对象的 `name` 完好无损。整个 const 对象被 move 时则完全退化为拷贝构造。
</details>

**4. `vector` 扩容时，为什么有移动构造却不一定用？**

<details><summary>答案</summary>
强异常保证：移动搬一半抛异常会破坏旧数据，拷贝失败则旧数据完好。标准库只在移动构造 `noexcept`（或类型不可拷贝）时才用移动。实测：无 `noexcept` 输出"拷贝"，有 `noexcept` 输出"移动"。
</details>

**5. 移动与拷贝代价相同的类型有哪些？**

<details><summary>答案</summary>
标量（int/指针）、POD 聚合体、`std::array`、SSO 范围内的短 `std::string`——它们的资源就在对象内，没有可交接的间接指针。
</details>

**6. "假定移动不存在、不廉价、不被用"分别对应什么情形？**

<details><summary>答案</summary>
不存在：遗留类/拷贝构造抑制移动生成；不廉价：`std::array`、标量、短字符串；不被用：const 对象/const 成员、`vector` 扩容遇非 `noexcept` 移动构造、`return std::move` 被 NRVO 规则架空之外的语义损失。
</details>

**7. 下面代码在性能上有什么隐患？**

```cpp
struct Config {
    const std::string venue;
    std::string notes;
    Config(const Config&) = default;
};
std::vector<Config> configs;   // 频繁 push_back
```

<details><summary>答案</summary>
双重隐患：①用户声明（default）拷贝构造会抑制移动构造生成，`configs` 扩容全部走拷贝；②即使补上移动构造，`const venue` 成员也只能拷贝。修正：去掉 `const`、`= default` 拷贝构造改为补 `Config(Config&&) noexcept = default;`，或直接用 `std::deque`/预留 `reserve` 规避扩容。
</details>

---

## 参考与延伸

- 上一节：[Item 28 引用折叠](item28-reference-collapsing.md)
- 下一节：[Item 30 完美转发失败的情形](item30-forwarding-failures.md)
- 关联：[Item 14 noexcept](../ch03-moving-to-modern-cpp/item14-noexcept.md)
- 回到：[第 5 章 右值引用、移动语义与完美转发](README.md)
