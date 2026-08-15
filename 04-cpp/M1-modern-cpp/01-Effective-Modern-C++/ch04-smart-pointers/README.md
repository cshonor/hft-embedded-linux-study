# 第 4 章 智能指针

**Smart Pointers** — Items 18–22

## 本章讲什么

裸 `new`/`delete` 是 C++ 资源泄漏与悬垂指针的头号来源。智能指针用 RAII 把"所有权"编码进类型系统：`unique_ptr` 表达独占、`shared_ptr` 表达共享、`weak_ptr` 打破环引用。本章讲清三者的所有权语义、性能代价与正确用法——这是 Modern C++ 区别于 C 的核心能力，也是 muduo / HFT 引擎回调与对象生命周期管理的基石。

---

## 各 Item 要点

### Item 18：用 `std::unique_ptr` 管理独占资源

`unique_ptr` 是**零开销抽象**——大小 = 裸指针（默认），开销 = 裸指针（移动是几条指令，析构内联调用 delete）。它表达"唯一所有权"，不可拷贝、只能移动。

```cpp
std::unique_ptr<Widget> make() { return std::make_unique<Widget>(); }
```

**自定义删除器**：`unique_ptr<Widget, void(*)(Widget*)>` 可挂自定义删除器。但删除器类型是 `unique_ptr` **类型的一部分**——不同删除器 = 不同类型。这对接口设计有影响（函数参数要明确删除器类型）。HFT 里用它管理 `FILE*`、`fd`、DPDK `rte_mbuf*`（自定义删除器调 `rte_pktmbuf_free`）。

**与 C API 互操作**：`unique_ptr` 的 `.get()` 取裸指针传给 C 接口；`.release()` 放弃所有权交还给 C。

### Item 19：用 `std::shared_ptr` 管理共享所有权

`shared_ptr` 用**引用计数**实现共享。控制块（control block）含强引用计数 + 弱引用计数 + 删除器 + 分配器。拷贝 `shared_ptr` 原子地递增计数（`++` 是原子操作），析构递减，归零调用删除器。

**代价**：
- 大小 = 2 个指针（对象指针 + 控制块指针），是裸指针的 2 倍。
- 拷贝/析构有**原子操作**开销——比非原子 `++` 慢一个数量级，且在多核间引发 cache 行同步。HFT 热路径**绝不**在每 tick 路径上拷贝 `shared_ptr`。

**控制块创建时机**（一次性，只创建一次）：
1. `make_shared`（推荐）
2. 从 `unique_ptr` 构造
3. 用裸指针 `new` 构造 `shared_ptr`——**两次 `new` 同一裸指针会创建两个控制块，double free**

**`enable_shared_from_this`**：对象需要在成员函数里返回自身的 `shared_ptr` 时，继承 `enable_shared_from_this<T>` 并调 `shared_from_this()`——直接 `shared_ptr<T>(this)` 会创建新控制块导致 double free。

### Item 20：用 `std::weak_ptr` 指向可能悬垂的 `shared_ptr`

`weak_ptr` 不增加强引用计数，观察 `shared_ptr` 但不延长对象生命。要访问对象须 `lock()` 提升为 `shared_ptr`（原子地检查并获取）：

```cpp
std::weak_ptr<Widget> wp = sp;
if (auto p = wp.lock()) { /* p 有效，安全使用 */ }
```

**三大用途**：缓存（对象可被回收）、观察者模式（观察者不持有被观察对象）、打破 `shared_ptr` 环引用（A↔B 互引导致计数永不归零、内存泄漏）。

### Item 21：优先 `make_unique` / `make_shared` 而非 `new`

`make_shared` 一次分配同时建对象 + 控制块（**单次堆分配**），比 `shared_ptr<T>(new T)` 的两次分配更省且 cache 友好。

**例外**（不能 `make` 的场景）：
- 需要自定义删除器 → 只能 `new`
- 自定义分配器
- `make_shared` 把对象内存与控制块绑在一起——若有 `weak_ptr` 长期存活，对象的内存（已析构但未释放）会**延迟到所有 weak_ptr 都销毁**才回收。大对象 + 长期 weak_ptr 场景要用 `new`。

### Item 22：用 Pimpl 惯用法降低编译依赖

Pimpl（Pointer to Implementation）把实现细节藏到 `.cpp`，头文件只留一个 `unique_ptr<Impl>`：

```cpp
// widget.h
class Widget {
    struct Impl;
    std::unique_ptr<Impl> pImpl;   // 不需要 Impl 的完整定义
public:
    Widget();
    ~Widget();   // 必须在 .cpp 定义（unique_ptr 析构需要完整类型）
};
```

头文件改动减少 → 编译依赖降低 → 增量编译加速。这和《C 和指针》ch07 的 opaque pointer / PIMPL 是同一思想，C++ 版用 `unique_ptr` 自动管理。

---

## HFT 关联

- **`unique_ptr` 管 mbuf / fd**：自定义删除器让 `unique_ptr` 管 DPDK `rte_mbuf*`（删除器调 `rte_pktmbuf_free`）或 POSIX `fd`（删除器调 `close`）。RAII 保证异常路径也不泄漏资源——比 C 的 `goto cleanup` 更安全且零运行开销。
- **`shared_ptr` 只在对象池/策略共享**：HFT 热路径不用 `shared_ptr`（原子计数代价），但对象池、策略对象（多线程共享、需要延迟销毁）用它管理生命周期。务必在**线程入口一次性拷贝** `shared_ptr`，热循环内用裸引用，避免每帧原子操作。
- **`weak_ptr` 观察者模式**：行情分发器持有策略的 `weak_ptr`，策略销毁后分发器 `lock()` 返回空自动跳过——避免策略热卸载时的悬垂回调。
- **Pimpl 降编译依赖**：大型交易引擎头文件改动会触发全量重编译，Pimpl 把核心结构藏到 .cpp，增量编译从分钟级降到秒级——开发效率的直接提升。

---

## 自测题

1. `unique_ptr` 和 `shared_ptr` 的大小分别是多少？为什么 `shared_ptr` 更大？
2. 用裸指针 `new` 构造两个 `shared_ptr` 会发生什么？`enable_shared_from_this` 如何解决成员函数返回自身的问题？
3. `make_shared` 比 `shared_ptr<T>(new T)` 省在哪里？什么场景下反而要用 `new`？
4. `weak_ptr` 如何安全访问对象？`lock()` 的原子性为什么重要？
5. Pimpl 里为什么析构函数必须在 `.cpp` 而非头文件定义？`unique_ptr` 析构对类型完整性的要求是什么？



## 代码自测

### Q1: shared_ptr double free

```cpp
Widget *raw = new Widget();
std::shared_ptr<Widget> p1(raw);
std::shared_ptr<Widget> p2(raw);  // 会发生什么？
```

> 创建两个 `shared_ptr` 指向同一裸指针，会发生什么？

<details>
<summary>答案与复习指引</summary>

**double free / 崩溃。** 每个 `shared_ptr` 创建自己的控制块，引用计数各为 1。两个都析构时各自 `delete raw` → double free。

**正确做法：** `auto p1 = std::make_shared<Widget>(); auto p2 = p1;`（拷贝共享同一控制块）。

**或用 `enable_shared_from_this`：** 对象继承 `enable_shared_from_this<T>`，在成员函数中调 `shared_from_this()` 返回正确的 `shared_ptr`。

**复习：** → [Item 19：用 shared_ptr 管理共享所有权](item19-shared-ptr.md)
</details>

### Q2: unique_ptr 自定义删除器

```cpp
std::unique_ptr<FILE, decltype(&fclose)> fp(fopen("data.txt", "r"), &fclose);
// fp 离开作用域后会发生什么？
```

> `fp` 离开作用域后如何处理 `FILE*`？

<details>
<summary>答案与复习指引</summary>

**自动调用 `fclose(fp.get())`。** `unique_ptr` 的第二个模板参数指定删除器类型，构造时传入删除器函数。RAII 保证即使异常路径也关闭文件。

**和 C 的区别：** C 需要手动 `fclose(fp)` 或 `goto cleanup`，异常路径容易遗漏。`unique_ptr` 把资源管理编码进类型系统。

**HFT 用途：** 管理自定义资源（`fd`/`mmap`/DPDK `mbuf`）——自定义删除器调 `close`/`munmap`/`rte_pktmbuf_free`。

**复习：** → [Item 18：用 unique_ptr 管理独占资源](item18-unique-ptr.md)
</details>
