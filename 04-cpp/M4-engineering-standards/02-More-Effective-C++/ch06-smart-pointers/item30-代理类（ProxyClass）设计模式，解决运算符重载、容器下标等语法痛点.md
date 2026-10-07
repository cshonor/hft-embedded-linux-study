# 条款 30：代理类（Proxy Class）——区分读写场景的语法艺术

## 本节讲什么

**Use proxy classes to distinguish lvalue/rvalue usage and control access.**
`operator[]` 有个经典难题：同一个语法 `v[i]`，读的时候该返回 const 引用，
写的时候可能要触发写时拷贝/校验/记账——一个函数怎么知道调用方是读是写？
答案：**不返回值，返回一个代理对象**，让"读"和"写"在代理的
不同运算符上分流。`std::vector<bool>` 的 `reference` 就是最著名的代理。

← 上一条 [item29 引用计数](./item29-实现引用计数智能指针（shared_ptr前身），循环引用问题来源就在这一条.md)；
下一章 [ch07 杂项](../ch07-miscellany/)。

---

## 1. 问题：`v[i]` 无法区分读和写

```cpp
template <typename T>
class SharedVec {                          // 写时拷贝（COW）容器
    std::shared_ptr<std::vector<T>> data_;
public:
    T& operator[](size_t i) { return (*data_)[i]; }     // 读也触发写路径？
    // v[3] = x 要写时拷贝（ detach ）；print(v[3]) 不该 detach——
    // 同一个 operator[] 怎么知道调用方要读还是写？答案：它不知道。
};
```

## 2. 代理解法：返回"待定的中间人"

```cpp
template <typename T>
class SharedVec {
    std::shared_ptr<std::vector<T>> data_;
public:
    class Proxy {                          // 代理对象：暂存"谁、哪个位置"
        SharedVec* owner_; size_t idx_;
    public:
        Proxy(SharedVec* o, size_t i) : owner_(o), idx_(i) {}
        // 读路径：隐式转换为 const T&（被读时走这里）
        operator const T&() const { return (*owner_->data_)[idx_]; }
        // 写路径：赋值运算符（被写时走这里）——触发 detach
        Proxy& operator=(const T& v) {
            owner_->detach();              // 写时拷贝：共享则先复制
            (*owner_->data_)[idx_] = v;
            return *this;
        }
    };
    Proxy operator[](size_t i) { return Proxy(this, i); }
private:
    void detach() { if (data_.use_count() > 1) data_ = std::make_shared<std::vector<T>>(*data_); }
};

// v[3] = x;   → Proxy::operator= → detach + 写        （写路径）
// print(v[3]); → Proxy::operator const T& → 直接读    （读路径，零 detach）
```

**原理**：`v[i]` 不再直接给答案，而是给一个**带着读写两套接口的中间人**——
调用方接下来的动作（隐式读取 = 转换运算符；赋值 = operator=）
决定了走哪条路径。区分读写从"编译期不可能"变成"运算符重载的自然分流"。

## 3. 经典实例与已知坑

**`std::vector<bool>`**：按位压缩存储，`operator[]` 返回 `reference` 代理
（读 = 取位；写 = 置位）——代理的正统案例，也是"代理不是真引用"的教训现场：

```cpp
std::vector<bool> v{true};
// bool& r = v[0];           // ❌ 编译错误：返回的是代理不是引用
auto r = v[0];               // r 的类型是 vector<bool>::reference（不是 bool!）
bool b = v[0];               // ✅ 经转换运算符读出
```

**代理的通用坑**：
- `auto` 会捕获**代理类型**而不是值类型（`auto x = v[0];` 拿到 Proxy——
  代理比容器短命时悬垂）；`auto` + 代理容器 = 时刻想着显式转换
- 代理对象的成员函数全集要手动维护（想让它"用起来像 T&"，每个操作都得转发）
- 链式/复合表达式里代理的生存期（`f(v[0] + v[1])`——两个临时代理的求值）

## 4. 适用与不适用

**适用**：读写语义确实不同（COW detach、写校验/审计、懒加载、远程数据）、
且调用形态是"取下标/取成员"的简单场景。

**不适用**（现代替代往往更好）：
- 只是为了"只读视图"——`std::span`/string_view（→ item54）零代理复杂度
- 读写都高频的热路径——代理多一层对象构造/转换（编译器通常能内联掉，
  但调试/异常路径上代理的类型会让错误信息变丑）
- 需要完整引用语义的场景（`&v[i]`、绑引用、decltype 推导）——代理全露馅

## HFT 关联

- COW 配置表（多策略共享默认配置，个别字段私有覆盖）是代理的正当场景：
  读高频零成本（隐式转换直通），写低频才 detach——
  但热路径读取要确保转换被内联（验证生成的代码，别假设）
- 订单簿的"逻辑下标→物理槽位"映射层用轻代理（下标校验 +  generation
  检查一体）——比每次手写 `book.levels_[idx]` 安全，比完整代理类简单
- `auto` 陷阱在高频代码里的防线：团队约定"代理容器处禁用 auto"
  （或全用 `const T& x = v[i];` 显式绑定）——代理类型泄漏到表达式里
  是 review 的高频漏网点

## 代码自测

**题目 1：** 代理类怎么把"读"和"写"分流到不同代码路径？

<details>
<summary>参考答案</summary>

靠**两个不同的运算符**接住调用方的下一步动作：
- 读：`operator const T&() const`（隐式转换）——
  `print(v[0])` 时编译器需要 const T&，转换运算符被选中（只读路径）；
- 写：`Proxy::operator=(const T&)`——`v[0] = x` 时赋值发生在**代理对象上**，
  代理的赋值运算符拿到写事件（触发 detach/校验/记账）。
`v[i]` 本身不给出答案，只构造一个"携带位置信息的中间人"——
读写分流从"编译期无法判断"变成"重载决议的自然结果"。
这就是代理模式的精髓：**把决策推迟到使用点**。

</details>

**题目 2：** `auto x = v[0];`（v 是代理容器）有什么坑？std::vector<bool> 的教训是什么？

<details>
<summary>参考答案</summary>

`auto` 推导的是 `operator[]` 的**返回类型**——代理类型
（`vector<bool>::reference` / 你的 Proxy），不是 `bool`/`T`：
① 后续代码以为拿到值（`x &= y;` 调到代理的 operator&=——
语义可能和预期完全不同）；
② 代理**比容器短命**的风险：临时容器 `get_vec()[0]` 的代理悬垂；
③ 类型泄漏：`decltype(x)`、模板实参推导全部拿到代理类型。
`vector<bool>` 的教训是：代理**不是真引用**——`bool& r = v[0];` 编译错误，
`auto& r = v[0];` 绑到临时代理（悬垂）。
防线：代理容器处禁用 auto（`bool x = v[0];` 或 `const bool& x = v[0];`
显式触发转换运算符），把"取值"动作显式化。

</details>

**题目 3：** 现代 C++ 里哪些"代理需求"有了更简单的替代？

<details>
<summary>参考答案</summary>

① **只读视图**：`std::span<const T>`/`string_view`——
零代理零转换，只读语义在类型层（→ item54 白名单）；
② **写时拷贝**：shared_ptr + 显式 `detach()`（把 detach 决策显式化）——
比隐式代理可读性好得多（写操作一眼可见）；
③ **懒加载**：`std::optional` + 显式 `value_or_compute()`——
懒语义显式，不靠转换运算符的隐式触发；
④ **下标校验**：轻量 `at()` 方法（抛/断言）——
比代理类简单一个量级。
代理剩下的正当场景：**读写分流是核心语义**（COW 的隐式 detach、
位压缩存储 vector<bool>、远程/ORM 式对象）——
且必须接受代理的全部代价（auto 陷阱/接口维护/类型泄漏）。
能用显式 API 表达的，就别用隐式代理——显式是现代 C++ 的总方向。

</details>
