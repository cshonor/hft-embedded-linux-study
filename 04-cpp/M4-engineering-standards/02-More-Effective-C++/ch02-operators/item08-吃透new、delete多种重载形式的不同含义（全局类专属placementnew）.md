# 条款 8：吃透 new、delete 多种重载形式的不同含义（全局/类专属/placement new）

## 本节讲什么

**Understand the different forms of new and delete.** "new" 这个词在 C++ 里
至少有四种身份：new 表达式、operator new（分配函数）、placement new
（在位构造）、以及它们的类专属重载。分不清这四者，就读不懂
19.1（两层模型）、Effective item49-52（类内重载/常规/placement 配对）——
本条是那条知识链的总入口（**深读链接**见各节，本条做辨析与地图）。

← 上一条 [item07 运算符重载](../ch02-operators/)；下一章 [ch03 异常](../ch03-exceptions/)。

---

## 1. 四种"new"的身份辨析

```cpp
// ① new 表达式（new-expression）：语言语法，不可重载
Widget* w = new Widget;
// 展开为两步（19.1 ① 两层模型）：
//   void* raw = operator new(sizeof(Widget));   // ← ② 分配函数：可重载！
//   new (raw) Widget;                            // ← ③ placement new：在位构造

// ② operator new（分配函数）：只搬内存，可调可重载
void* p = ::operator new(100);        // 全局版——只管分配，不构造（malloc 的 C++ 形）
::operator delete(p);

// ③ placement new：不分配，在指定内存上构造
alignas(Widget) char buf[sizeof(Widget)];
Widget* q = new (buf) Widget;         // 构造在 buf 上（19.1 ③ 铁律：手动析构，永不 delete）
q->~Widget();

// ④ 类专属 operator new：单类型重载（Effective item49）
class Order {
    static void* operator new(std::size_t n) { return order_pool().acquire(); }
    static void operator delete(void* p) noexcept { order_pool().release(p); }
};
```

**核心区分**：①是**语法**（编译器展开），②③④是**函数**（可替换的实现件）——
"重载 new"重载的永远是②（或带参数的③变体），①的语法本身动不了。

## 2. 全局 vs 类专属 vs placement（对照表）

| 形态 | 作用域 | 何时被调 | 深读 |
|---|---|---|---|
| 全局 `::operator new` | 全程序 | `new T` 且类内无重载 | 19.1 ②（实测埋点） |
| 类专属 `T::operator new` | 单类型（+派生） | `new T`（优先于全局） | Effective item49 |
| 标准 placement（void*） | 语言自带 | `new (buf) T` | 19.1 ③ |
| 自定义 placement（额外参数） | 你定义 | `new (pool, tag) T` | Effective item52（配对义务！） |
| 数组版 `operator new[]` | 独立一族 | `new T[n]` | item51 ③：与普通版互不相干 |

**delete 的镜像规则**：每种 new 都有对应的 delete 形态
（全局/类专属/placement/数组）——配对错误（placement new 的对象走 delete、
数组走非数组 delete）是 UB 的经典来源。

## 3. 名称查找的连锁反应（item51 ③ 的复习）

类内声明**任何** operator new → 隐藏全局**全部**重载形式：

```cpp
class Widget {
    static void* operator new(std::size_t n);      // 只声明了这个
};
// new (buf) Widget;      // ❌ 编译错误：全局 placement new 被藏掉了！
// 要用必须类内也声明 placement 版（→ Effective item52 的完整常规）
```

## HFT 关联

- 池化体系（item18 freelist 池）就是④的实战：类专属 operator new 接池，
  `new Order` 的语法不动、底层从 malloc 变 freelist（→ 19.1 HFT 的零分配纪律）
- placement new（③）是无锁队列/环形缓冲的构造原语（→ 19.1 ③ 的 SpscRing 骨架）——
  热路径"push"的本质 = placement 构造 + 序号发布
- 四种形态的辨析在**审查内存相关 bug**时是定位起点：
  "这个分配走的是哪条路？"——全局埋点（19.1 ② 实测）一次说清

## 代码自测

**题目 1：** 以下四种 new 各是什么含义？

```cpp
// 1
Widget* p = new Widget;
// 2
void* mem = ::operator new(sizeof(Widget));
new(mem) Widget;  // 这是什么 new？
// 3
class Widget {
    static void* operator new(size_t, ostream& log);  // 这是什么？
};
// 4
```

<details>
<summary>参考答案</summary>

① **new 表达式**（语法）——展开为 operator new 分配 + 构造函数两步；
② **placement new**（标准 void* 版）——`::operator new(sizeof)` 只分配裸内存
（没构造！mem 里是无对象的字节），`new (mem) Widget` 在其上**在位构造**——
两步拆开写，就是 19.1 ① 两层模型的手工版；
③ **自定义 placement new**（带 ostream& 额外参数的分配函数重载）——
调用形如 `new (std::clog) Widget`（日志埋点的经典形态，
→ Effective item50 ② 的调用点记录思路）；注意按 Effective item52，
写了它就必须写镜像的 `operator delete(void*, ostream&)`（构造抛异常时回收）；
④ 未给出——预留位通常指**数组版** `operator new[]`（`new Widget[n]` 走它，
与普通版独立，→ Effective item51 ③）。

</details>

**题目 2：** `void* p = ::operator new(100);` 之后能 `delete (Widget*)p;` 吗？

<details>
<summary>参考答案</summary>

**不能**——两个错误：
① `::operator new(100)` 分配的是**裸内存**（没有对象被构造），
`delete` 会先调析构函数——对"不是对象的字节"调析构 = UB；
② 配对错误：operator new 分配的内存要用 **operator delete** 归还
（`::operator delete(p);`），`delete` 表达式是"析构 + operator delete"的
组合（19.1 ①）——用于未构造的内存是语义错配。
正确配对：`::operator new` ↔ `::operator delete`（裸内存层）；
`new T` ↔ `delete p`（对象层）；`new (buf) T` ↔ 手动 `p->~T()`（在位层，
buf 的寿命归调用方，19.1 ③ 铁律）。三层的配对各不相通。

</details>

**题目 3：** 池化 Order 的类内 operator new 写好后，`new (slot) Order` 为什么编译失败？

<details>
<summary>参考答案</summary>

**名称遮掩**（Effective item33 的规则在 operator new 上的实例）：
类内声明 `static void* operator new(size_t)` 后，类作用域的查找在此停止——
全局的 placement new（`void* operator new(size_t, void*)`）被隐藏，
`new (slot) Order` 的候选集里只剩你类内那版（签名不符）→ 编译错误。
修法：类内补声明 placement 版——
`static void* operator new(std::size_t, void* p) noexcept { return p; }`
（→ Effective item52 的类内三重注意：placement 配对 + 遮掩 + 继承传播）。
这也是"重载了 operator new 就要想全套"的原因之一——
你接管的不只是分配，还有整个 new 家族的查找规则。

</details>
