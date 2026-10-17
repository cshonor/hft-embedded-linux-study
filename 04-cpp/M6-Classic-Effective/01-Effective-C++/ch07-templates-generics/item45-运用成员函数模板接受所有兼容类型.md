# 条款 45：运用成员函数模板接受所有兼容类型

## 本节讲什么

**Use member function templates to accept "all compatible types."**
`shared_ptr<T>` 能从 `shared_ptr<U>`（U 是 T 的派生）构造——但两个
`shared_ptr` 是**不同的类**，普通的"拷贝构造"帮不上忙。答案：
**成员函数模板**（member template）——类模板里的模板构造函数，
让"兼容类型之间"的构造/赋值成为可能。智能指针是这个手法最标准的示范。

← 上一条 [item44 抽离参数无关代码](./item44-将与参数无关的代码抽离templates.md)；
下一条 [item46 模板非成员函数的类型转换](./item46-需要类型转换时请为模板定义非成员函数.md)。

---

## 1. 问题：类模板实例之间不是"同一个类"

```cpp
std::shared_ptr<Base> pb = std::make_shared<Derived>();
// shared_ptr<Derived> 和 shared_ptr<Base> 是两个**毫不相干**的类——
// 不是"同一个类的两个对象"，拷贝构造/赋值根本不在候选集
```

`shared_ptr<Derived>` 的拷贝构造只接受 `const shared_ptr<Derived>&`——
`shared_ptr<Base>` 连边都沾不上。但语义上"派生指针"转"基类指针"天经地义，
需要一座桥。

## 2. 解法：成员函数模板（智能指针的真身）

```cpp
template <typename T>
class SmartPtr {
    T* ptr_;
public:
    explicit SmartPtr(T* p) : ptr_(p) {}

    // 成员函数模板：接受"所有兼容类型"的桥
    template <typename U>
    SmartPtr(const SmartPtr<U>& other,
             typename std::enable_if_t<std::is_convertible_v<U*, T*>>* = nullptr)
        : ptr_(other.get()) {
        // U* → T* 可隐式转换（即 U is-a T）才放行——
        // enable_if 把"兼容"钉在类型系统里（C++20 用 requires 更干净）
    }
    T* get() const { return ptr_; }
};
SmartPtr<Derived> pd(new Derived);
SmartPtr<Base> pb = pd;      // ✅ 成员模板构造命中
// SmartPtr<Derived> pd2 = pb;   // ❌ Base* → Derived* 不可转，enable_if 拦截
```

**要点**：
- 成员模板**不抑制**编译器生成的拷贝构造——两者共存，
  同类型拷贝走拷贝构造（更特化），跨类型走成员模板
- `enable_if`（或 C++20 `requires`）把"兼容"从口头约定变成编译期约束——
  只允许"指针可隐式转换"的方向（派生→基类），反向直接出局
- `shared_ptr`/`unique_ptr`/`weak_ptr` 全是这套写法——
  本条就是读标准库智能指针源码的说明书

## 3. 同样手法在容器/包装器上的推广

```cpp
template <typename T>
class Optional {
    // ...
    template <typename U>
    Optional(const Optional<U>& other,
             std::enable_if_t<std::is_constructible_v<T, const U&>>* = nullptr);
    // 只要 U 能构造出 T，Optional<U> 就能转 Optional<T>
};
```

判别什么时候需要成员模板：**你的类持有/包装一个 T，
且"T 的兼容家族"（派生/可转换）应该被一体接纳**——
指针包装（is-a 链）、值包装（可构造链）、引用包装都适用。

## HFT 关联

- 自定义句柄/包装（`PoolPtr<T>`/`FeedHandle<T>`）需要同样的桥：
  `PoolPtr<SnapMsg>` → `PoolPtr<FeedMsg>` 的转换构造必须走成员模板 + enable_if——
  消息层次（item32 的消息继承）在指针包装层才能保持一致
- 注意"兼容"的语义必须**自己定义**：`is_convertible_v<U*, T*>` 是指针世界的答案；
  值语义世界（Price/Tick）要换成 `is_constructible_v` 或自定义 concept——
  别照搬智能指针的约束到不相干的场景
- C++20 起 `requires` 让这类构造的报错从 enable_if 的"无候选"变成
  "不满足约束：U* 必须可转换为 T*"——模板库的可读性红利（→ item41 concepts）

## 代码自测

**题目 1：** `shared_ptr<Derived>` 转 `shared_ptr<Base>` 为什么不能用拷贝构造？

<details>
<summary>参考答案</summary>

`shared_ptr<Derived>` 和 `shared_ptr<Base>` 是**两个不同的类**
（模板按不同参数实例化出无关类型）——`shared_ptr<Base>` 的拷贝构造
声明是 `shared_ptr(const shared_ptr<Base>&)`，实参类型 `shared_ptr<Derived>`
根本不匹配，连重载决议的候选集都进不了。
跨实例转换必须靠**成员函数模板**（`template <typename U> shared_ptr(const shared_ptr<U>&)`）——
它对任何 U 都能生成一个"构造函数"，再用 enable_if/requires 把方向
限制在"U* 可隐式转换为 T*"（派生→基类）。

</details>

**题目 2：** 成员模板构造函数会抑制编译器生成的拷贝构造吗？

<details>
<summary>参考答案</summary>

**不会**。成员**模板**不是拷贝构造——即使它的某个实例化在签名上恰好
等价于拷贝构造，编译器仍会照常生成拷贝构造/赋值。
重载决议时：同类型实参优先匹配**非模板**的拷贝构造（非模板优先于模板），
跨类型实参才落到成员模板。
这正是想要的分工：同族走高效拷贝，跨族走兼容性检查的成员模板——
两者互补，互不干扰。

</details>

**题目 3：** 给自己的 `PoolPtr<T>` 写跨类型转换构造，C++20 写法是什么？

<details>
<summary>参考答案</summary>

```cpp
template <typename T>
class PoolPtr {
    T* ptr_;
public:
    explicit PoolPtr(T* p) : ptr_(p) {}
    template <typename U>
        requires std::convertible_to<U*, T*>        // 约束即文档
    PoolPtr(const PoolPtr<U>& other) : ptr_(other.get()) {}
    T* get() const { return ptr_; }
};
```

`requires std::convertible_to<U*, T*>` 一行顶替 enable_if 模板参数——
不满足约束时错误信息直接是"约束不满足：U* 不可转换为 T*"，
而不是"没有匹配的构造函数"+一长串候选。
消息包装层的 is-a 链（PoolPtr<SnapMsg> → PoolPtr<FeedMsg>）由此获得
与智能指针一致的转换语义（→ item41 concepts 前瞻落地）。

</details>
