# 条款 19：设计类等同于设计一种全新类型

## 本节讲什么

**Treat class design as type design.** 在 C++ 里 **`class` = 新类型**；应像语言设计者规划 `int` 一样规划自定义类型：**语法自然、语义直觉、实现高效**。定义任何类之前，先回答下面 **12 个问题**——它们覆盖构造/销毁、拷贝/赋值、不变量、继承、转换、接口边界与通用性。

← [条款 18：接口易用难误用](./item18-接口设计要易用正确、难被误用.md) · [条款 20：const 引用传参 →](./item20-优先使用const引用传参，而非值传递.md)

---

## 设计心态

| 内置类型 `int` | 你的类 `Widget` |
|--------------|----------------|
| 如何创建/销毁？ | ctor / dtor / `operator new`？ |
| 拷贝意味着什么？ | copy ctor / move ctor |
| 哪些值合法？ | **不变量（invariants）** |
| 能隐式转成什么？ | conversion / `explicit` |
| 客户能依赖什么保证？ | **未声明接口**（异常安全、线程、复杂度） |

**类设计 = 类型设计 = 一份完整契约**，不是「随便加几个成员函数」。

---

## 12 个必答问题

### 1. 对象如何被创建和销毁？

决定：**构造函数、析构函数**；是否在堆上特殊分配 → **`operator new` / `delete` / `new[]` / `delete[]`**（[条款 16](../ch03-resource-management/item16-new和delete成对使用，形式保持一致.md)）。

```cpp
class Widget {
public:
    Widget();
    ~Widget();
    static void* operator new(std::size_t size);
    static void  operator delete(void* p) noexcept;
    // 数组形式若需要：operator new[] / delete[]
};
```

- 栈对象 vs 堆对象 vs 池化分配，策略不同。  
- 工厂返回智能指针时，销毁路径仍须一致（[条款 18](./item18-接口设计要易用正确、难被误用.md)）。

---

### 2. 初始化与赋值有何不同？

| | 初始化 | 赋值 |
|---|--------|------|
| **时机** | 对象**诞生**时 | 对象**已存在** |
| **函数** | 构造函数 | `operator=` |
| **典型** | `Widget w(arg);` | `w = other;` |

**绝不能混为一谈**——未初始化成员就赋值 vs 先析构再拷贝，语义与实现都不同（[条款 10](../ch02-constructors-destructors-assignment/item10-令operator=返回引用指向this对象.md)、[条款 11](../ch02-constructors-destructors-assignment/item11-在operator=中处理对象自我赋值.md)）。

---

### 3. 按值传递意味着什么？

**Copy constructor**（及 C++11 的 **move constructor**）定义「传值」的语义：

```cpp
void f(Widget w);   // 调用 copy/move ctor 构造 w 的副本
```

- 深拷贝资源？引用计数？禁止拷贝？（[条款 6](../ch02-constructors-destructors-assignment/item06-不想用编译器自动生成的函数，就明确禁用.md)、[条款 14](../ch03-resource-management/item14-资源管理类谨慎设计拷贝行为.md)）  
- 大对象常改为 **`const Widget&`**（[条款 20](./item20-优先使用const引用传参，而非值传递.md)），但**类型仍须定义**拷贝/移动语义。

---

### 4. 合法值有何限制？（不变量）

成员组合中哪些必须恒真？例如：

- `Month` 必须在 1–12  
- `BankAccount` 余额不为 NaN；透支上限  
- 容器 `size() <= capacity()`

→ 在 **ctor、setter、`operator=`** 里校验；非法输入 **抛异常** 还是 **断言**，须事先决定。  
→ 与 [条款 18](./item18-接口设计要易用正确、难被误用.md) 强类型、`Month::Jan()` 同源。

---

### 5. 是否适合放进继承体系？

| 角色 | 须考虑 |
|------|--------|
| **作为派生类** | 基类接口约束；哪些函数 `virtual` / `non-virtual` 不可随意改语义 |
| **作为基类** | 是否被多态删除？→ **虚析构**（[条款 7](../ch02-constructors-destructors-assignment/item07-为多态基类声明virtual析构函数.md)） |
| **是否应继承** | 有时 **组合优于继承**；不要为了复用而 public 继承 |

构造/析构中调虚函数的限制见 [条款 9](../ch02-constructors-destructors-assignment/item09-绝不在构造和析构过程中调用virtual函数.md)。

---

### 6. 允许哪种类型转换？

| 目标 | 手段 |
|------|------|
| **隐式** `T1 → T2` | `T1::operator T2()` 或 `T2(T1)` 非 `explicit` 单参 ctor |
| **仅显式** | `explicit` ctor；或 `static_cast` / 命名函数 `toT2()` |

```cpp
class Fraction {
public:
    explicit operator double() const;  // C++11 起可 explicit 转换函数
};
```

默认倾向 **少隐式、多 explicit**（[条款 24](./item24-需要所有参数都支持隐式类型转换时，使用非成员函数.md) 讨论何时需要隐式）。

---

### 7. 哪些运算符和函数有意义？

- 算术？比较？流输出？  
- **成员 vs 非成员**：[条款 23](./item23-优先用非成员、非友元函数替代成员函数.md) — 对称运算符（`==`、`+`）常为非成员。  
- 返回值 `const` 防误用（[条款 18](./item18-接口设计要易用正确、难被误用.md) `operator*` 示例）。

只声明**有清晰语义**的操作；没有的不要凑。

---

### 8. 哪些标准函数应禁用？

编译器可能自动生成：**copy ctor、copy assign、move、dtor**。

不需要或危险时：

```cpp
class LockHandle {
public:
    LockHandle(const LockHandle&) = delete;
    LockHandle& operator=(const LockHandle&) = delete;
};
```

原书写 **private 且不实现**；现代 C++ 用 **`= delete`**（[条款 6](../ch02-constructors-destructors-assignment/item06-不想用编译器自动生成的函数，就明确禁用.md)）——**误用 = 编译错误**，优于链接期错误。

---

### 9. 哪些成员可被谁访问？

- **`public`**：客户契约，改动代价高  
- **`protected`**：对子类开放，仍破坏封装风险  
- **`private`** + 访问器（[条款 22](./item22-成员变量一律声明为private.md)）  
- **`friend`**：仅当非成员函数**必须**访问实现细节时慎用

接口越小，误用面越小（条款 18）。

---

### 10. 「未声明接口」提供什么保证？

文档与实现中客户**会依赖**、但未必写在函数签名里的承诺：

| 维度 | 示例问题 |
|------|----------|
| **异常安全** | 强保证 / 基本保证 / 不抛？ |
| **线程安全** |  const 成员是否可并发读？ |
| **复杂度** | `size()` O(1)？ |
| **资源** | 是否持锁、是否分配堆内存 |

这些保证**约束实现**；一旦发布，变更 = 破坏客户代码。RAII 与 [条款 13](../ch03-resource-management/item13-以对象管理资源（RAII）.md) 常是未声明接口的一部分。

---

### 11. 需要多大程度的通用性？

单一 `Array` 不够 → 考虑 **`template<typename T>`** 一族类型：

```cpp
template<typename T, std::size_t N>
class FixedArray { /* ... */ };
```

- 类模板 = **类型族设计**，问题 1–10 对每个 `T` 仍适用。  
- 非必要勿模板化：编译时间、错误信息、二进制体积（HFT 常权衡）。

---

### 12. 真的需要全新类型吗？

| 需求 | 可能更轻的方案 |
|------|----------------|
| 给 `string` 加一点算法 | **非成员函数** `trim(s)` |
| 扩展行为、多态 | **组合** 或 **私有继承** 实现细节 |
| 强类型包装 | **`using OrderId = strong_typedef<...>`** 薄封装 |

新类有 **维护成本**；先问能否用现有类型 + 自由函数 / 别名 / 派生解决。

---

## 12 问速查表

| # | 问题 | 主要落点 |
|---|------|----------|
| 1 | 创建/销毁 | ctor, dtor, `operator new/delete` |
| 2 | 初始化 vs 赋值 | ctor vs `operator=` |
| 3 | 按值传递 | copy/move ctor |
| 4 | 合法值 / 不变量 | 校验、异常 |
| 5 | 继承 | 虚析构、组合 vs 继承 |
| 6 | 类型转换 | `explicit`、转换函数 |
| 7 | 运算符/函数 | 成员 vs 非成员 |
| 8 | 禁用函数 | `= delete` |
| 9 | 访问控制 | public/private/friend |
| 10 | 未声明接口 | 异常安全、性能、线程 |
| 11 | 通用性 | 类模板 |
| 12 | 是否必要 | 非成员函数、派生、组合 |

---

## HFT 视角

- **OrderBook / Tick**：先写清不变量（价位有序、数量非负），再写 ctor/setter。  
- **热路径类型**：问 3（按值 vs 引用）、问 10（分配、锁、异常）——许多行情结构 **`= delete` 拷贝**、只移动。  
- **插件接口**：问 5 + 7 + 9——基类虚析构、最小 `public` 面、工厂出 `shared_ptr`。  
- 新类评审：**12 问 checklist** 过一遍再合并。

---

## 易错点速查

| 坑 | 对应问题 |
|----|----------|
| 把 `operator=` 当构造用 | 2 |
| 有资源却无 copy/move 规则 | 3、8 |
| 基类无 virtual dtor | 5 |
| 单参 ctor 意外隐式转换 | 6 |
| 全 public 数据成员 | 9 |
| 未文档化「不会抛异常」 | 10 |
| 为一种 `T` 写死，马上又要 `U` | 11 |
| 为一个小功能造 500 行 class | 12 |

---

## 核心总结

1. **定义 class = 定义新类型**，标准应接近内置类型。  
2. 动手写代码前，**12 个问题**逐项有答案。  
3. 创建/销毁、初始化/赋值、拷贝语义是**语法可见**的骨架；不变量与未声明接口是**语义与可靠性**的骨架。  
4. 继承、转换、运算符决定**类型在系统中的位置**；`= delete` 与访问级别决定**误用成本**。  
5. 能不用新类就不用——但**一旦定义，就设计完整**（衔接 [条款 18](./item18-接口设计要易用正确、难被误用.md) 易用难误用、[条款 22](./item22-成员变量一律声明为private.md) 封装）。

← [条款 18：接口易用难误用](./item18-接口设计要易用正确、难被误用.md) | [条款 20：const 引用传参 →](./item20-优先使用const引用传参，而非值传递.md)
