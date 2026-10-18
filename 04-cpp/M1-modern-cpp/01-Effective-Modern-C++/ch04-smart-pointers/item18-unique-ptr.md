# Item 18：用 std::unique_ptr 管理独占资源

> 第 4 章 智能指针 · Item 18 · 下一节：[Item 19 shared_ptr](item19-shared-ptr.md)

## 为什么要学这个（先建立直觉）

C 里管理堆资源靠纪律：`malloc` 之后配对 `free`，中间有 `goto cleanup`、有异常路径、有早退 `return`——每一条路都得记得释放，漏一条就是泄漏。`unique_ptr` 把这条纪律变成了**类型系统强制的规则**：资源包进对象，对象出作用域自动释放，所有权唯一、拷贝直接被编译器拒绝。

更关键的是 Meyers 强调的卖点：**零开销**。`unique_ptr` 不是"方便但慢一点"的妥协——默认配置下它和裸指针一样大、一样快，是"不用白不用"的纯收益抽象。这一节用实测把这个"零开销"拆开看清楚，包括它在什么情况下**不再零开销**（有状态删除器）。

---

## 这节讲什么

1. 零开销的实测证据：`sizeof` 对比、移动语义、拷贝被删的报错原文；
2. 自定义删除器：类型成为 `unique_ptr` 一部分的含义，以及删除器对**体积**的影响（EBO）；
3. 与 C API 互操作：`.get()` / `.release()` / `.reset()` 的所有权语义；
4. 数组特化 `unique_ptr<T[]>` 与 `make_unique`。

---

## 核心用法与零开销实测

```cpp
#include <memory>

std::unique_ptr<Widget> make() {
    return std::make_unique<Widget>();   // 工厂返回 unique_ptr：所有权随返回值移交
}
// 离开作用域自动 delete，异常路径也释放——RAII
```

clang 23.1.0（arm64）实测尺寸：

```
sizeof(unique_ptr<int>)                    = 8     ← 和裸指针完全一样
sizeof(unique_ptr<FILE, decltype(&fclose)>)= 16    ← 函数指针删除器：+8
sizeof(unique_ptr<int, 无捕获lambda>)       = 8     ← 无状态删除器：EBO 压掉了
sizeof(unique_ptr<int, 有捕获lambda>)       = 16    ← 有状态删除器：要存状态
```

两个要点直接写在数字里：

- **默认删除器（`delete`）零成本**：8 字节 = 一个裸指针。解引用就是指针解引用，析构就是内联 `delete`，无任何计数、无任何间接层。
- **删除器占不占空间取决于它有没有状态**：无捕获 lambda / 空函数对象是"无状态删除器"，靠空基类优化（EBO）压成 0 字节；函数指针（`&fclose`）和有捕获 lambda 要把删除器**存进对象**，所以 +8 字节。

### 移动可以，拷贝不行

```cpp
auto p1 = std::make_unique<int>(42);
auto p2 = std::move(p1);
// 实测：p1 == nullptr 为 true，*p2 == 42 —— 所有权是"移交"不是"共享"

auto p3 = p1;   // 编译失败
```

clang 报错原文：

```
error: call to implicitly-deleted copy constructor of 'unique_ptr<int>'
```

拷贝构造被显式 `= delete`——"唯一所有权"不是约定，是编译期强制。想转移就 `std::move`，意图明明白白写在代码里。

---

## 自定义删除器：类型的一部分

```cpp
std::unique_ptr<FILE, decltype(&fclose)> fp(fopen("f.txt", "r"), &fclose);
// 离开作用域自动 fclose
```

**删除器类型是 `unique_ptr` 类型的第二个模板参数**——`unique_ptr<T, D1>` 和 `unique_ptr<T, D2>` 是不同类型，不能互相赋值、不能放进同一个容器。推论：

- 函数指针删除器：`unique_ptr<T, void(*)(T*)>`，16 字节；
- lambda 删除器：每个 lambda 都是独一无二的匿名类型，写进函数签名要靠 `auto`/模板/`decltype`；
- 管 C 资源的标准手法：`fclose` 管 `FILE*`、`close` 管 fd、库自带的 `xxx_free` 管库句柄。

### 与 C API 互操作的所有权三件套

```cpp
std::unique_ptr<Widget> up = std::make_unique<Widget>();

raw_observe(up.get());        // 借出裸指针：所有权不动，up 还负责释放
Widget* p = up.release();     // 交出所有权：up 置空，p 归你管（忘了 delete 就泄漏）
up.reset(new Widget);         // 先释放当前对象，再接管新对象
```

一句话：`.get()` 是**借**，`.release()` 是**给**，`.reset()` 是**换**。传给只读接口用 `.get()`；传给"之后归 C 库管"的接口用 `.release()`。

### 数组特化

```cpp
auto arr = std::make_unique<int[]>(1024);   // unique_ptr<int[]>，析构调 delete[]
arr[0] = 1;                                 // 有 operator[]，无 operator*/->
```

`unique_ptr<T[]>` 自动配对 `delete[]`，避免 `new[]`/`delete` 错配。但能用它的地方先想想 `std::vector`——vector 有尺寸、有迭代器、能进算法，`unique_ptr<T[]>` 只在"必须对接要裸数组所有权的 C API"时有存在价值。

---

## 常见错误（新手踩坑）

1. **用裸 `new` 构造而不是 `make_unique`**。`unique_ptr<Widget> up(new Widget);` 能编过，但 `make_unique` 更短、且避免"`new` 表达式求值与 `unique_ptr` 构造之间抛异常导致泄漏"的经典窗口（C++14 起有 `make_unique`，没有理由不用）。
2. **同一裸指针构造两个 `unique_ptr`**：`Widget* p = new Widget; unique_ptr<Widget> a(p), b(p);` → 双重释放。规则：**裸指针交给智能指针后，立刻忘掉这个裸指针**。
3. **移动后还使用源指针**。`std::move` 之后源 `unique_ptr` 是 `nullptr`（实测确认），解引用即崩溃。`move` 过的指针只剩两种合法操作：重新赋值、析构。
4. **以为函数指针删除器也零开销**。实测 16 字节——要省体积，把删除器写成无捕获 lambda 或空函数对象。

---

## 新手要点（和 C 的区别）

- **C 用 malloc/free + goto cleanup**：释放靠人肉，`return` 早退、`longjmp`、错误分支每条都要记得清理。`unique_ptr` 把释放绑在栈对象生命周期上，任何退出路径都自动执行——这是 RAII，也是 C++ 相对 C 在资源管理上最实质的升级。
- **零开销不是口号**：默认 `unique_ptr` 与裸指针同大同快（实测 8 字节），优化后汇编通常与手写 `new/delete` 逐指令一致。"抽象不交税"是它成为默认选择的原因。
- **不可拷贝是特性不是缺陷**：拷贝被 `= delete`，强制单一所有权；要共享请显式换 `shared_ptr` 并付引用计数的税（见 [Item 19](item19-shared-ptr.md)）。

---

## HFT 关联

- **管 mbuf / fd / 库句柄**：自定义删除器让 `unique_ptr` 管 DPDK `rte_mbuf*`（删除器调 `rte_pktmbuf_free`）、POSIX fd（删除器调 `close`）、库句柄。RAII 保证异常路径、早退路径都不泄漏——比 `goto cleanup` 干净，还零开销。
- **热路径资源的安全网**：热路径上传递所有权用 `unique_ptr` + `std::move`，无引用计数、无原子操作——这点和 `shared_ptr` 形成决定性对比。
- **工厂函数返回 `unique_ptr`**：`std::unique_ptr<Strategy> make_strategy(Config)` 是策略对象创建的标准签名——调用方拿到独占所有权，接口语义自文档化。

---

## 自测题

1. 默认 `unique_ptr<int>` 多大？凭什么说"零开销"？什么情况下它会变成 16 字节？
2. `unique_ptr<FILE, decltype(&fclose)>` 和 `unique_ptr<FILE, 某lambda类型>` 是同一类型吗？能放进同一个 vector 吗？
3. `.get()` / `.release()` / `.reset()` 各是什么所有权语义？
4. 为什么 `unique_ptr` 不可拷贝但可移动？移动后源对象处于什么状态？
5. `Widget* p = new Widget; unique_ptr<Widget> a(p), b(p);` 会发生什么？根因是什么？
6. `unique_ptr<T[]>` 和 `std::vector<T>` 怎么选？

<details>
<summary>参考答案</summary>

1. 8 字节（64 位，实测）。"零开销"指与手写 `new`/`delete` 相比不引入额外内存或时间成本：无引用计数、无控制块、解引用与析构都是内联的指针操作，独占所有权完全靠类型系统（删除拷贝）在编译期表达。变成 16 字节的两种情况：删除器是函数指针（要存指针），或删除器是有捕获 lambda / 有状态函数对象（要存状态）。无捕获 lambda 靠 EBO 仍是 8 字节。

2. 不是同一类型——删除器类型是第二个模板参数，删除器不同则 `unique_ptr` 类型不同，不能互相赋值，也不能进同一个 `vector<unique_ptr<T, D>>`。两个不同的 lambda 类型也互不相同（每个 lambda 都是唯一匿名类型）。

3. `.get()` 借出裸指针，所有权不动（智能指针仍负责释放），适合传给只观察的接口；`.release()` 交出所有权并置空自身，释放责任转给接收方，忘了 `delete` 就泄漏；`.reset(p)` 先释放当前持有的对象再接管 `p`。记忆法：借 / 给 / 换。

4. 语义是独占所有权：拷贝会产生两个"唯一"拥有者，析构时双重释放，所以拷贝构造/拷贝赋值被 `= delete`（实测报错 `call to implicitly-deleted copy constructor`）。移动是所有权转移：源对象把指针交出后置空（实测 `p1 == nullptr` 为 true），全程只有一个拥有者。移动后的源对象处于"合法但未指定"状态，只剩重新赋值和析构两种安全操作。

5. 双重释放（UB）。两个 `unique_ptr` 各自以为独占 `p`，析构时各 `delete` 一次。根因是裸指针没有所有权信息，构造 `unique_ptr` 时编译器无法知道这个裸指针已经被别的智能指针持有。纪律：裸指针一交给智能指针就不再使用；优先 `make_unique`，让裸指针根本没有具名存在的机会。

6. 默认 `std::vector<T>`：有尺寸、有迭代器、能进标准算法、会自己扩容。`unique_ptr<T[]>` 只在需要"拥有并移交一块裸数组的所有权"时考虑（对接 C API、手写内存池），它无尺寸信息、只能下标访问，是更接近裸 `new[]` 的薄封装。

</details>

---

## 参考与延伸

- 下一节：[Item 19 shared_ptr](item19-shared-ptr.md)
- 回到：[第 4 章 智能指针](README.md)
