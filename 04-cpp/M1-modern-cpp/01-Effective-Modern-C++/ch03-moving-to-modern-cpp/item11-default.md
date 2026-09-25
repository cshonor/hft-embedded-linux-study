# Item 11：优先 = default 声明默认构造

> 第 3 章 移步现代 C++ · Item 11 · 上一节：[Item 10 scoped enum](item10-scoped-enum.md)

## 为什么要学这个（先建立直觉）

C 的结构体没有构造函数，编译器自动处理初始化：

```c
struct Point { int x; int y; };
struct Point p = {0};   // 零初始化，x=0, y=0
// 或者
struct Point p;         // 未初始化，值不确定
memset(&p, 0, sizeof(p));  // 手动清零，安全
```

C++ 有了构造函数。当你写了一个构造函数，编译器就不再自动生成默认构造。如果你想恢复编译器的默认行为，C++11 之前只能手写空函数体：

```cpp
struct A {
    int x;
    A(int v) : x(v) {}    // 自定义构造
    A() {}                 // 手写空默认构造——但这有副作用！
};
```

问题是：手写的 `A() {}` 会变成"用户定义的构造函数"，这会改变 `A` 的类型性质——从 trivial 变成 non-trivial，影响 `memcpy` 合法性和 ABI 传递效率。

`= default` 让你显式表达"我要编译器的默认行为"，不改变 trivial 性质：

```cpp
struct B {
    int x;
    B(int v) : x(v) {}
    B() = default;   // 编译器生成——B 保持 trivial（如果成员都是 trivial）
};
```

---

## 这节讲什么

`= default` 让编译器生成默认实现，比手写空函数体更可靠——手写 `C() {}` 会变成"用户定义"，影响是否为 trivial 类型（影响 `memcpy` 合法性与 ABI）。

---

## 核心区别

### trivial vs non-trivial

```cpp
struct A {
    int x;
    A() {}              // 用户定义：A 不是 trivial
};
struct B {
    int x;
    B() = default;      // 编译器生成：B 是 trivial（成员 int 是 trivial）
};
struct C {
    int x;
    // 什么都不写：编译器自动生成默认构造，C 是 trivial
};

// 验证
static_assert(std::is_trivial_v<A> == false);   // A 不是 trivial
static_assert(std::is_trivial_v<B> == true);     // B 是 trivial
static_assert(std::is_trivial_v<C> == true);     // C 是 trivial
```

trivial 类型的优势：
- 可以安全 `memcpy` / `memset`
- 可以作为 `union` 成员
- ABI 边界传递更高效（可以直接按值传递，不需要构造/析构代码）
- 编译器可以做更激进的优化

### 什么时候用 = default

```cpp
struct Order {
    int64_t id;
    double price;
    int64_t qty;

    // 场景 1：写了自定义构造，想恢复默认构造
    Order(int64_t i, double p, int64_t q) : id(i), price(p), qty(q) {}
    Order() = default;  // 恢复默认构造

    // 场景 2：拷贝/移动操作也想用编译器默认
    Order(const Order&) = default;
    Order(Order&&) = default;
    Order& operator=(const Order&) = default;
    Order& operator=(Order&&) = default;

    // 场景 3：禁用某个操作
    Order(const Order&) = delete;  // 禁止拷贝
};
```

---

## 常见错误（新手踩坑）

**错误 1：手写空构造破坏 trivial 性质**
```cpp
struct Packet {
    char data[64];
    Packet() {}  // 以为和默认行为一样，实际 Packet 变成 non-trivial
};
// memcpy(&pkt, src, 64);  // 技术上合法但行为未定义（non-trivial 类型）
```
**修正：** `Packet() = default;` 或什么都不写。

**错误 2：写了构造函数后忘了恢复默认构造**
```cpp
struct Config {
    std::string name;
    Config(const std::string& n) : name(n) {}
    // 没写默认构造——Config() 不可用！
};
Config c;  // 编译失败：没有默认构造
```
**修正：** 加 `Config() = default;`（注意：如果成员有非默认构造的，`= default` 生成的函数可能被删除）。

**错误 3：对非 trivial 成员用 = default 以为能 memcpy**
```cpp
struct Bad {
    std::string s;      // string 不是 trivial
    Bad() = default;    // 生成默认构造，但 Bad 仍不是 trivially copyable
};
// memcpy(&bad, src, sizeof(bad));  // 未定义行为！string 内部有指针
```
**修正：** `= default` 保持 trivial 的前提是**所有成员都是 trivial**。含 `string`/`vector` 等的类型不能 `memcpy`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 默认初始化 | 编译器自动处理 | 有构造函数后需显式恢复 | C++ 有构造函数机制 |
| 空构造 | 不存在 | `= default` vs `() {}` | 前者保持 trivial，后者破坏 |
| trivial 性质 | 所有 struct 都 trivial | 取决于构造函数写法 | 影响 memcpy/ABI |
| 禁用操作 | 不存在 | `= delete` | C++ 的访问控制 |

**一句话总结：** C 程序员记住——C++ 里手写空构造 `A() {}` 不等于"什么都不做"，它会改变类型性质。想用编译器默认行为就写 `= default`。

---

## HFT 关联

- **POD 类型**：HFT 协议结构体用 `= default` 保持 trivial，可用 `memcpy` 直接操作网络缓冲区。
- **`memcpy` 合法性**：trivially copyable 类型才能安全 `memcpy`——手写空构造会破坏这一性质。
- **缓存行对齐**：trivial 类型的布局更可预测，配合 `alignas(64)` 可以精确控制缓存行对齐。

---

## 自测题

1. `A() {}` 和 `A() = default;` 有什么区别？哪个保持 trivial 性质？
2. 为什么 trivial 类型可以用 `memcpy` 而 non-trivial 不行？
3. 什么场景需要手写构造函数而非 `= default`？
4. 下面代码有什么问题？
```cpp
struct Msg {
    std::string topic;
    char buf[64];
    Msg() {}  // 这里会有什么隐患？
};
```
5. `struct X { int a; std::string b; X() = default; };` 是 trivially copyable 吗？为什么？

<details>
<summary>参考答案</summary>

1. `A() {}` 是**用户提供的**（user-provided）构造函数，即使函数体为空，它也使类型不再是 trivially default constructible、不再 trivial、不再是 POD；`A() = default;`（写在类内、首次声明即 defaulted）则是显式要求编译器生成默认实现，编译器生成的版本不算 user-provided，因此**保持 trivial / POD 性质**。所以保持 trivial 的是 `A() = default;`。另外 `= default` 的默认构造通常还是 `constexpr`/`noexcept` 友好的，且比手写空函数更容易被优化。

2. `memcpy` 的前提是"对象的对象表示就是它的值表示"：trivially copyable 类型没有用户自定义的拷贝/移动/析构，字节序列复制到另一个同类型对象的存储后，得到的是一个等价的有效对象。non-trivial 类型（含 `std::string`、虚函数、用户定义拷贝构造等）内部持有资源指针/不变量，逐字节复制会产生两个对象共享同一资源（双重释放、悬垂指针），或破坏类不变量，因此标准规定对 non-trivially-copyable 类型用 `memcpy` 是未定义行为。

3. 需要建立类不变量或非零初始化的场景：成员需要非默认初值（`Member m{42};`）、需要申请资源/注册/初始化锁、需要保证成员被值初始化（内置类型默认构造时是不确定的，需要 `int x = 0;`）、需要 `explicit` 或有副作用（日志、统计）。这些都无法用 `= default` 表达，只能手写。

4. 隐患是：`Msg() {}` 是用户提供的构造函数，它对 `std::string topic` 会调用默认构造（这没问题），但对 `char buf[64]` **不做任何初始化**——数组元素是未定义值。若把 `Msg` 序列化发到网络或写日志，就会泄漏之前缓冲区里的残留数据（典型的信息泄漏 + 不可复现的脏数据 bug）。修正：用 `Msg() = default;` 加成员默认初始化（`char buf[64]{};`），或在初始化列表里 `Msg() : buf{} {}`。

5. **不是** trivially copyable。决定性因素是 `std::string` 成员：`std::string` 自身有用户定义的拷贝构造、移动构造和析构函数，因此它不是 trivially copyable 类型；一个类只要有任何非 trivially copyable 的非静态成员，它自己就不是 trivially copyable。`X() = default;` 只保证默认构造是平凡的，不影响拷贝相关性质（而且这里默认构造实际上也不平凡——`std::string` 的默认构造本身非 trivial）。

</details>

---

## 参考与延伸

- 下一节：[Item 12 override](item12-override.md)
- 回到：[第 3 章 移步现代 C++](README.md)
