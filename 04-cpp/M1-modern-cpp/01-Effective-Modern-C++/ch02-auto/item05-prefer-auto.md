# Item 5：优先用 auto 而非显式类型声明

> 第 2 章 auto · Item 5 · 下一节：[Item 6 显式类型初始化习惯](item06-explicitly-typed-initializer.md)

## 为什么要学这个（先建立直觉）

很多 C 转 C++ 的人把 `auto` 理解成"少打几个字母的语法糖"，于是态度是"可读性优先，简单类型我还是手写"。这个理解漏掉了重点：`auto` 的价值不在省键盘，而在**它能做到手写类型做不到的事**——有些类型你根本写不出来（lambda 闭包），有些类型你写了就是错的（窄化截断），有些类型你写完就要维护（重构跟随）。Meyers 这一条的真正立场是：**默认 auto，有理由才显式**。

当然，Item 5 和 Item 6 是一对：Item 5 说"优先 auto"，Item 6 立刻补"但 auto 有一种危险场景"。两条一起读才是完整规则。

---

## 这节讲什么

`auto` 在四个维度上的真实优势，全部实测：

1. **正确性**：杜绝未初始化变量、杜绝隐式窄化截断；
2. **可维护性**：重构时类型自动跟随；
3. **能力**：接住你根本写不出名字的类型（lambda 闭包）；
4. **性能**：`auto` 接 lambda 与 `std::function` 接 lambda 的开销差距。

---

## 优势一：杜绝未初始化变量

`auto` 的类型从初始化器推导——**没有初始化器就没有推导来源**，于是：

```cpp
int x;      // 合法：未初始化，值不确定（UB 预备役）
auto x;     // 编译失败
```

clang 23.1.0 报错原文：

```
error: declaration of variable 'x' with deduced type 'auto'
      requires an initializer
```

`auto` 从语法层面封死了"声明了忘初始化"这条路径。C 里 `int x;` 带着栈上的随机垃圾值被用掉，是最经典的随机 bug 来源；C++ 的 `auto` 声明想忘都忘不了。

## 优势二：杜绝隐式窄化

手写类型最大的风险是**你以为的类型和实际的类型不一致**，编译器静默插入转换：

```cpp
#include <cstdint>
#include <iostream>

int main() {
    uint64_t real = 5000000000ULL;   // 50 亿，超过 UINT_MAX
    unsigned u = real;               // 静默截断
    auto a = real;                   // uint64_t，原样

    std::cout << "unsigned: " << u << "\n";     // 705032704 ← 错了！
    std::cout << "auto:     " << a << "\n";     // 5000000000
    std::cout << sizeof(a) << "\n";             // 8 字节
}
```

clang 23.1.0 实测输出：

```
unsigned: 705032704     ← 50 亿被截成 7 亿，无任何警告
auto:     5000000000
8
```

5000000000 mod 2³² = 705032704。手写 `unsigned` 拿到一个**看起来合理、实际错误**的值——这是最危险的 bug 形态：不崩、不警告、数字还挺像回事。经典版本是 `unsigned sz = vec.size();`（`size_t` 64 位 → `unsigned` 32 位）。

## 优势三：重构时类型自动跟随

```cpp
auto it = book.find(symbol);   // book 从 unordered_map 换成 map，
                               // iterator 类型变了，这行零改动
```

显式写法 `std::unordered_map<std::string, Order, HashFn>::iterator it = ...` 在容器换实现、返回类型从 `iterator` 变 `const_iterator`、值类型从 `int` 变 `int64_t` 时，每一处都要跟着改——改漏一处就是一次隐式转换或编译错误。`auto` 把"类型是什么"的决定权交给定义处，使用处自动跟随，这正是"不要重复你自己"在类型系统里的样子。

## 优势四：接住写不出名字的类型 + 零开销

lambda 表达式的类型是编译器合成的**唯一匿名闭包类型**，没有名字可写：

```cpp
auto lam = [x](int n){ return x * n; };
std::cout << typeid(lam).name() << "\n";   // Z4mainE3$_0 ← 编译器内部名，你写不出
```

你只能三选一接住 lambda：`auto`、模板参数、`std::function`。前两者零开销，`std::function` 是类型擦除，有真实成本（clang 23.1.0 实测）：

```cpp
int x = 3;
auto lam = [x](int n){ return x * n; };
std::function<int(int)> fn = lam;

sizeof(lam)   // 4   字节（就是一个捕获的 int）
sizeof(fn)    // 32  字节（类型擦除的控制块，还可能堆分配）
```

闭包本身 4 字节，`std::function` 包装后 32 字节，且调用经过间接跳转，捕获多了还会触发堆分配。性能敏感场景（回调注册、热路径策略）用 `auto`/模板，别用 `std::function`。

---

## 常见错误（新手踩坑）

1. **`auto` 当动态类型理解**。`auto x = 42; x = "str";` 编译失败——`auto` 是编译期静态推导，推导完类型就钉死了，和 Python 的变量毫无关系。
2. **该用 auto 的地方手写长类型名**。`std::unordered_map<std::string, Order>::const_iterator it = m.cbegin();` 又臭又长还容易写错（写成 `iterator` 就编译失败），`auto it = m.cbegin();` 一行解决。
3. **以为 `auto` 会推成引用**。`auto x = getRef();` 是拷贝（Item 2 按值形态）。要引用写 `auto&`——`auto` 全家桶里这是最高频的坑。
4. **数字字面量配 auto 推错宽度**。`auto flags = 0;` 是 `int`；想要 `uint64_t` 就写 `auto flags = uint64_t{0};` 或 `0ULL`。`auto` 只推你写的字面量的类型，不会读你的心。

---

## 新手要点（和 C 的区别）

- **C 必须手写类型**，`int x = 42;` 是唯一选择。C++ 里 `auto x = 42;` 等价且防窄化，但简单类型手写意图更直白——`int count = 0;` 一眼看清，`auto count = 0;` 要读者自己推。
- **决策规则**（实用版）：类型名长或写不出（迭代器、lambda、make_xxx 返回值）→ `auto`；类型简单且你想让读者立刻看到（计数器、循环变量）→ 手写；怕窄化/怕类型漂移（`size()`、序列号）→ `auto`。
- **`auto` 不等于 `var`**：不是动态类型，不是类型擦除，编译期钉死，运行时零开销。

---

## HFT 关联

- **订单数量截断是资损级 bug**：`int q = order.qty();` 在 `qty()` 返回 `int64_t` 时静默截断——上面 50 亿变 7 亿的实测就是这类 bug 的模型。下单数量、序列号、时间戳一律 `auto` 或显式定宽类型（`int64_t`）。
- **lambda 回调注册**：`auto cb = [this](const Tick& t){ ... };` 是策略引擎标准写法。用 `std::function` 存回调等于在热路径上白交 8 倍体积 + 间接调用 + 潜在堆分配。
- **时间戳别手写 `long`**：`long` 在 Linux x86_64 是 64 位、Windows 是 32 位。`auto ts = exch.timestamp();` 或 `uint64_t`——跨平台的手写整型是移植性地雷。

---

## 自测题

1. `auto x;` 为什么编译不过？这条规则消灭了哪一类经典 C bug？
2. `unsigned sz = 5000000000ULL;` 得到什么？`auto sz = 5000000000ULL;` 呢？实测各是多少？
3. 为什么 lambda 的类型无法手写？接住 lambda 的三种方式里哪种有运行时开销？
4. 实测一个只捕获一个 `int` 的 lambda 闭包多大？`std::function` 包装后多大？
5. 举一个"重构时 auto 比显式类型省事"的例子。
6. `auto x = getRef();`（`getRef` 返回 `int&`）中 `x` 是引用吗？想拿引用该怎么写？

<details>
<summary>参考答案</summary>

1. `auto` 的类型从初始化器推导，没有初始化器就无从推导，`auto x;` 报 `requires an initializer`。这从语法上封死了"声明忘初始化"——C 里 `int x;` 带随机栈值被使用是最经典的随机 bug 来源，用 `auto` 想犯都犯不了。

2. `unsigned sz` 静默截断为 `5000000000 mod 2³² = 705032704`（实测输出），编译器不警告；`auto sz` 推为 `uint64_t`（`unsigned long long`），得到完整的 `5000000000`，`sizeof` 8 字节。前者拿到一个"看起来合理、实际错误"的值，是最危险的 bug 形态。

3. lambda 的类型是编译器合成的唯一匿名闭包类型，没有可书写的名字（`typeid` 看到的是 `Z4mainE3$_0` 这种内部符号）。三种接法：`auto`（零开销）、模板参数（零开销）、`std::function`（类型擦除，有间接调用 + 体积 + 潜在堆分配开销）。

4. 闭包 4 字节（就装一个捕获的 `int`），`std::function` 32 字节（clang 23.1.0 / libc++ 实测）——8 倍体积，调用还多一次间接跳转。

5. 把容器从 `std::unordered_map<std::string, Order>` 换成 `std::map<std::string, Order>`：显式写法里每一处 `unordered_map<...>::iterator it = m.find(k);` 都要改；`auto it = m.find(k);` 一处不用动。同理函数返回类型 `int` → `int64_t`，显式调用方全要改，`auto` 自动跟随还顺便消灭了截断。

6. 不是。`auto` 按值形态剥掉引用，`x` 是 `int` 的拷贝，改 `x` 不影响原对象。要拿引用写 `auto& x = getRef();`（或 `decltype(auto)`）。这是 `auto` 全家桶里最高频的坑——见 [Item 2](../ch01-deducing-types/item02-auto-type-deduction.md) 按值形态。

</details>

---

## 参考与延伸

- 下一节：[Item 6 显式类型初始化习惯](item06-explicitly-typed-initializer.md)
- 回到：[第 2 章 auto](README.md)
