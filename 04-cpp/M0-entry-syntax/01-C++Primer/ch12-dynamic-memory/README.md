# 第 12 章 动态内存

本章探讨动态内存的分配和管理，重点推荐 C++11 **智能指针**机制，提高代码鲁棒性并避免内存泄漏。

## 小节

- [智能指针](./12.1-智能指针.md)
- [直接内存管理](./12.2-直接内存管理.md)
- [动态数组与 allocator](./12.3-动态数组与allocator.md)
- [实战：文本查询程序](./12.4-实战：文本查询程序.md)


## 章节摘要

动态内存管理：智能指针 `shared_ptr`（共享所有权）、`unique_ptr`（独占所有权）、`weak_ptr`（弱引用不增加引用计数），以及直接内存管理 `new`/`delete`、动态数组与 `allocator`。

### 和 C 的区别

| C | C++ |
|---|-----|
| `malloc`/`free` | `new`/`delete`（调用构造/析构） |
| 手动管理生命周期 | RAII 智能指针自动释放 |
| 无弱引用 | `weak_ptr` 打破环引用 |
| `valgrind` 查泄漏 | 智能指针从源头避免泄漏 |

## 章节自测

### Q1: shared_ptr 引用计数

```cpp
auto p = std::make_shared<int>(42);
auto q = p;
std::cout << *p << " " << *q << " " << p.use_count();
p.reset();
std::cout << " " << q.use_count();
```

> 输出是什么？`reset()` 做了什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `42 42 2 1`

**解析：**
- `p` 和 `q` 共享同一对象，引用计数 = 2
- `p.reset()` 释放 `p` 的引用，引用计数降为 1。对象不被销毁（`q` 仍持有）
- `q.use_count()` = 1

**注意：** `shared_ptr` 的引用计数操作是**原子的**——线程安全但有多核同步开销。HFT 热路径避免频繁拷贝 `shared_ptr`。

**复习：** → [智能指针](./12.1-智能指针.md)
</details>

### Q2: unique_ptr 独占

```cpp
// std::unique_ptr<int> p1 = std::make_unique<int>(42);
// std::unique_ptr<int> p2 = p1;  // A: 合法吗？
std::unique_ptr<int> p3 = std::move(p1);  // B: 合法吗？
// p1 现在是什么状态？
```

> A 合法吗？B 合法吗？`p1` 在 B 之后是什么状态？

<details>
<summary>答案与复习指引</summary>

**A: 编译错误。** `unique_ptr` 不可拷贝（独占所有权）。
**B: 合法。** `unique_ptr` 可以移动——移动后 `p3` 拥有对象，`p1` 变为空（`nullptr`）。

**和 `shared_ptr` 的区别：** `shared_ptr` 可拷贝（共享），`unique_ptr` 只可移动（独占）。`unique_ptr` 是零开销——大小 = 裸指针，无引用计数。

**复习：** → [智能指针](./12.1-智能指针.md)
</details>

### Q3: weak_ptr 打破环引用

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;  // A: 环引用？
    // std::weak_ptr<Node> prev; // B: 修复方案
};
auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->prev = a;
// a 和 b 离开作用域后...
```

> 用 A 行（`shared_ptr` prev）时，a 和 b 离开作用域后内存会释放吗？B 行如何修复？

<details>
<summary>答案与复习指引</summary>

**用 A 行：不会释放。** `a` 离开作用域，引用计数从 2→1（`b->prev` 仍持有 `a`）；`b` 离开作用域，引用计数从 2→1（`a->next` 仍持有 `b`）。互引导致计数永远不为 0——内存泄漏。

**用 B 行（`weak_ptr`）：会释放。** `weak_ptr` 不增加强引用计数。`a` 离开作用域时引用计数→0（`b->prev` 是 `weak_ptr` 不算），`a` 被释放；然后 `b` 引用计数→0，`b` 被释放。

**规则：** 双向引用时，一方用 `shared_ptr`（强引用），另一方用 `weak_ptr`（弱引用）打破环。

**复习：** → [智能指针](./12.1-智能指针.md)
</details>

### Q4: new vs make_shared

```cpp
// 方式 A:
std::shared_ptr<Widget> p1(new Widget());
// 方式 B:
auto p2 = std::make_shared<Widget>();
```

> A 和 B 有什么区别？B 好在哪里？

<details>
<summary>答案与复习指引</summary>

**B 好在：**
1. **一次分配**：`make_shared` 一次 `new` 同时分配对象 + 控制块；方式 A 两次 `new`（对象 + 控制块）
2. **cache 友好**：对象和控制块在同一块内存
3. **异常安全**：`f(shared_ptr<T>(new T), g())` 中 `g()` 可能在 `new` 和 `shared_ptr` 构造之间抛异常；`f(make_shared<T>(), g())` 不会

**B 的缺点：** `make_shared` 把对象内存与控制块绑定，如果有 `weak_ptr` 长期存活，对象内存（已析构但未释放）会延迟到所有 `weak_ptr` 销毁才回收。大对象 + 长期 `weak_ptr` 场景要用 A。

**复习：** → [智能指针](./12.1-智能指针.md) · [直接内存管理](./12.2-直接内存管理.md)
</details>
