# 条款 49：了解 class-specific 的 new/delete

## 本节讲什么

**Understand class-specific new and delete.** 全局 `operator new`（→ 19.1 ② 实测）
影响面是全程序；类内重载则只作用于**单个类型**——
它是"给某个高频小对象配专用池"的语言机制。本条讲清类内重载的形态、
继承时的传播规则、以及与容器分配器的分工。

← 上一条 [item48 new-handler](./item48-了解new-handler的行为.md)；
下一条 [item50 替换 new/delete 的时机](./item50-了解new和delete的合理替换时机.md)。

---

## 1. 类内重载的形态与效果

```cpp
class Order {
    // 类内 operator new/delete：new Order 时优先用它们（静态成员语义，不写 static 也是）
public:
    static void* operator new(std::size_t n) {
        return order_pool().acquire();     // 专用池：freelist 弹出，O(1) 无锁（单线程域）
    }
    static void operator delete(void* p) noexcept {
        order_pool().release(p);
    }
    static void operator delete(void* p, std::size_t) noexcept {   // C++14 sized delete
        order_pool().release(p);           // 池不用存 size（→ 19.1 sized delete 的红利）
    }
    // ...
};
Order* o = new Order;      // → 走类内重载（池化）
delete o;                  // → 归还池
// int* p = new int;       // → 不受影响，仍走全局
```

**要点**：
- 类内重载是**隐式静态**的（写不写 `static` 都是静态语义）——
  它在构造函数运行**之前**执行（成员还不存在），同理 delete 在析构之后
- `new Order[n]` 走的是 `operator new[]`——类内**数组版要单独重载**，否则数组分配漏回全局
- 隐藏规则：类内声明 `operator new` 会**藏掉**全局的所有重载（含 placement new——
  要用 `new (ptr) Order` 必须类内也声明 placement 版，→ item52）

## 2. 继承传播（容易踩的坑）

```cpp
class IocOrder : public Order {};
IocOrder* io = new IocOrder;    // → 也走 Order 的类内重载！（重载随继承传播）
```

- 类内 operator new **被继承**——派生类对象也进基类的池
- 若派生类尺寸不同（`sizeof(IocOrder) > sizeof(Order)`），按 Order 尺寸设计的池
  可能放不下——池要按**最大尺寸**分档，或派生类重载覆盖自己的版本
- 判别：单类型池的成员必须**尺寸一致**——继承体系里尺寸会变的类型，
  池化方案改走"按尺寸分档的池"或容器分配器

## 3. 与容器分配器的分工（别混用两套）

| 机制 | 粒度 | 适用 |
|---|---|---|
| 类内 operator new/delete | 单类型（+派生） | **裸 new 的高频类型**（Order/Tick 手工 new 的场景） |
| 容器 Allocator（ch12） | 单容器实例 | **容器内元素**（`vector<T, PoolAlloc<T>>`） |
| 全局重载 / LD_PRELOAD | 全程序 | 统计/换分配器（jemalloc） |

常见错误：`vector<Order>` 里的元素**不走** Order 的类内重载——
vector 的元素内存是 vector 自己的 allocator 一次性分配的，
元素的"new"根本不存在（placement new 构造，→ 19.1 ③）。
想让容器元素池化，走 **Allocator** 而不是类内重载。

## HFT 关联

- **订单/回报对象的池化**是类内重载的标准场景：每tick成百上千的
  `new Order` 走 freelist 池（O(1) 无分配器锁）——运行期零 malloc 的第一步
  （→ 19.1 HFT、06.6.5 ch07 分配器抖动）
- sized delete（19.1 实测）让池省掉 per-object 元数据头——类内重载时
  记得**两个 delete 版本都写**（sized + 非 sized）
- 类内重载 + 继承陷阱：订单类型族（IocOrder/LimitOrder…）尺寸不一——
  要么禁继承池化（各类型各配池），要么统一基类池 + 按最大尺寸分档
- 与 `-fno-exceptions` 配套：池 `acquire()` 失败返回 nullptr 还是 terminate
  要在类内重载里显式定义——全局 bad_alloc 语义在池化世界不存在

## 代码自测

**题目 1：** 类内 `operator new` 为什么必须是静态语义？它在什么时候运行？

<details>
<summary>参考答案</summary>

`new Order` 的完整流程是：先 `Order::operator new(sizeof(Order))` 拿内存，
**然后**才在这块内存上运行构造函数（→ 19.1 ① 两层模型）。
分配时**对象还不存在**——没有 this，没有成员，operator new 只能是静态的
（类内声明写不写 `static` 关键字，语义都是静态成员函数）。
delete 同理镜像：先析构（对象死了），再 `operator delete` 释放内存。
这条时序决定了：operator new/delete 里**不能访问任何非静态成员**。

</details>

**题目 2：** `class IocOrder : public Order {}` 且 Order 有类内 operator new——
`new IocOrder` 走哪里的分配？有什么坑？

<details>
<summary>参考答案</summary>

走 **Order 的类内 operator new**——类内重载随 public 继承传播。
坑：池通常按 `sizeof(Order)` 设计块大小，而 `sizeof(IocOrder)` 可能更大
（加了成员）——按小块分配的内存装大对象 = 溢出/损坏。
对策三选一：① 派生类重载自己的 operator new（各尺寸各池）；
② 池按继承体系**最大尺寸**统一分档（浪费但简单）；
③ 禁止池化类型被继承（`final`）——尺寸钉死，池假设成立。
池化的第一条戒律是"块尺寸可预期"，继承体系天然威胁这条戒律。

</details>

**题目 3：** `std::vector<Order>` 的元素会走 Order 的类内 operator new 吗？为什么？

<details>
<summary>参考答案</summary>

**不会**。vector 的内存管理是**两层**（→ 19.1 ①）：
分配层——vector 用自己的 allocator 一次性申请 `n * sizeof(Order)` 的裸缓冲；
构造层——元素用 **placement new** 在缓冲上构造。
整个过程中不存在 `new Order` 表达式，类内 operator new 的钩子**不会被触发**。
想让容器元素池化，正确机制是**自定义 Allocator**（`vector<Order, OrderPoolAlloc>`，
→ ch12）——它接管的是 vector 的分配层，与类内重载是两套独立机制。
混用期望（"类内重载了，vector 也该走池"）是这套机制最常见的误解。

</details>
