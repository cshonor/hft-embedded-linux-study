# Item 11a：对 STL 容器的线程安全抱有现实的期望

> 第 1 章 容器 · Item 11a（原书 Item 12，补遗） · 上一节：[Item 10-11 分配器](item10-11-allocators.md)

## 为什么要学这个（先建立直觉）

C 程序员面对共享数组，本能反应是"加锁"——但加多大、加在哪、谁来加，全都自己定。
换到 C++，很多人会**默认 STL 容器自带某种线程保护**，毕竟它封装得那么好：

```cpp
std::vector<Order> book;   // 两个线程同时操作
// 线程 A：book.push_back(o);      // 写
// 线程 B：if (!book.empty()) x = book.back();  // 读
// → 数据竞争，UB。vector 不会帮你挡任何一下
```

现实是：**STL 容器的线程安全保证比你想象的少得多**——Meyers 这条 Item 就是泼这盆冷水的。

## 这节讲什么

STL（以及后来的 C++ 标准）对容器只给两条最低保证，其余全是你的责任：

1. **多个线程同时读同一个容器**（全部 const 操作）——安全；
2. **多个线程同时写不同的容器**——安全。

除此之外，**同读同写、同写同写同一个容器 = 数据竞争 = UB**，标准库一概不管。

## 标准到底保证什么

SGI STL 的原始表述（C++11 基本原样继承）：

| 场景 | 保证 |
|------|------|
| N 个线程并发读同一容器 | ✅ 安全 |
| N 个线程并发写 N 个**不同**容器 | ✅ 安全 |
| 读 + 写同一容器 | ❌ 数据竞争，UB |
| 写 + 写同一容器 | ❌ 数据竞争，UB |

两个容易误读的细节：

- **「读」指 const 操作**。`v[i]`（非 const 重载）、`push_back`、`erase`、`begin()` 返回的非 const
  迭代器解引用后修改——全是写。`operator[]` 即使只是"读值"，走的也是非 const 重载时就别赌。
- **实现可以给你更强的保证，但你不能依赖**。MSVC 调试版的迭代器检查、libstdc++ 的
  `vector<bool>` 位操作——都是实现细节，跨平台就翻车。

## 你必须自己做的三件事

**1. 修改时上锁，读也要上锁（如果与写并发）**

```cpp
std::mutex m;
std::vector<Order> book;

// 写
{ std::lock_guard lk(m); book.push_back(o); }

// 读 —— 只锁「看起来像读」还不够，复合操作必须整体锁
Order top;
{ std::lock_guard lk(m);
  if (!book.empty()) top = book.back(); }   // empty+back 必须原子
```

**2. 用 RAII 保证异常路径也解锁**——`lock_guard`/`scoped_lock`/`unique_lock`，
永远别手动 `m.lock() / m.unlock()` 裸奔（异常一跳就死锁）。

**3. 读写都热时考虑「拷出来改完再换」**（copy-on-write 模式）：

```cpp
// 读多写少：写方先拷贝整份，改完原子换指针
auto cur = std::atomic_load(&book_ptr);        // 读方零锁
auto nxt = std::make_shared<Book>(*cur);       // 写方：拷
nxt->push_back(o);                             //       改副本
std::atomic_store(&book_ptr, nxt);             //       换
```

## HFT 关联

- **HFT 的第一反应不是加锁，是消灭共享**：SPSC ring buffer、每线程一份数据、
  单写者原则——共享容器压根不出现，这条 Item 的所有问题就不存在。
  上锁永远是方案 B。
- **实在要共享**：配置表、合约表这类读极多写极少的数据，用上面的
  COW + `shared_ptr` 原子换指针，读路径零锁零系统调用。
- **别把「实现好像没崩」当保证**：x86 TSO 下很多数据竞争「碰巧能跑」，
  换 ARM64 弱序平台立刻现形——和 CCiA ch05 内存模型笔记里
  `ldar`/`stlr` 缺一条就崩是同一个教训。

## 代码自测

### Q1：下面这段哪里有问题？

```cpp
std::vector<int> v = {1, 2, 3};
std::mutex m;

void reader() {
    std::lock_guard lk(m);
    std::cout << v[0];            // 读
}
void writer() {
    std::lock_guard lk(m);
    v.clear();                    // 写
    std::cout << v[0];            // ← 这行呢？
}
```

<details>
<summary>答案与复习指引</summary>

`writer` 里 `clear()` 之后立刻 `v[0]`——**锁只防线程间竞争，防不了自己逻辑错误**：
`clear()` 后容器为空，`v[0]` 越界 UB（`operator[]` 不做边界检查）。

线程安全 ≠ 逻辑安全。锁保证的是"操作期间没有别的线程进来"，
不保证"这个操作本身合法"。空容器访问要用 `.at(0)`（抛异常）或先查 `empty()`。

复习：锁的职责边界——互斥只管并发，不管语义。
</details>

### Q2：N 个线程各自 `push_back` 到自己的 `vector`，需要锁吗？

<details>
<summary>答案与复习指引</summary>

**不需要**——保证 2：并发写**不同**容器是安全的。
这正是「每线程一份数据」模式的合法性基础：thread-local 的 vector 随便写，
最后 join 后归并即可。

注意陷阱：这些 vector 必须真的是不同对象——如果它们是同一个
`vector<vector<int>>` 的元素，外层 vector 扩容导致元素搬迁时，
各线程持有的引用/指针会失效，那又回到「同一容器」问题上了。

复习：保证 2 的适用条件是「不同对象」，不是「不同名字」。
</details>

### Q3：为什么说「x86 上碰巧能跑」不能作为线程安全证据？

<details>
<summary>答案与复习指引</summary>

x86 是 TSO 强内存序：绝大多数 store 对其他核按发出顺序可见，
编译器和 CPU 重排的窗口小，很多缺失同步的代码「碰巧」表现正常。
ARM64 是弱序：`load`/`store` 可大幅重排，没有 `ldar`/`stlr`（acquire/release）
屏障，读到撕裂数据、看到"未来的"指针都是常态。

数据竞争是 **UB**——行为依赖硬件、编译器、时序。能通过测试只说明
「这次没触发」，不说明「正确」。正确性只能来自内存模型层面的论证
（或 TSan 这类工具的动态验证）。

复习：CCiA ch05 内存模型；调试方法论「先让它对（TSan），再让它快」。
</details>

## 延伸

- 配套：CCiA ch03（共享数据保护：mutex、死锁、`shared_mutex`）
- 进阶：无锁路线见 CCiA ch06/ch07；COW 模式的现代形式见
  Effective Modern C++ Item 16（const 成员函数的线程安全）
- C 程序员迁移：C 里"全局数组 + 大锁"的思维 → C++ 里先问
  「能不能不共享」（thread-local / SPSC），再问「锁多大」（粒度），最后才是「用什么锁」
