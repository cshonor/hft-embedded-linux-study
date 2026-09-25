# Item 19：用 std::shared_ptr 管理共享所有权

> 第 4 章 智能指针 · Item 19 · 上一节：[Item 18 unique_ptr](item18-unique-ptr.md)

## 这节讲什么

`shared_ptr` 用**引用计数**实现共享。代价：大小 = 2 个指针（对象 + 控制块），拷贝/析构有**原子操作**开销。理解控制块是安全使用 `shared_ptr` 的关键。

---

## 核心机制

控制块（control block）含：强引用计数 + 弱引用计数 + 删除器 + 分配器。

```cpp
auto sp = std::make_shared<Widget>();  // sp 引用计数 = 1
auto sp2 = sp;                          // 拷贝，计数 = 2（原子 ++）
// sp2 销毁 → 计数 = 1
// sp 销毁 → 计数 = 0 → delete Widget
```

### 控制块创建时机（只创建一次）

1. `make_shared`（推荐）—— 单次分配
2. 从 `unique_ptr` 构造
3. 用裸指针 `new` 构造 `shared_ptr`

**致命错误**：两次 `new` 同一裸指针构造两个 `shared_ptr` → 两个控制块 → **double free**：
```cpp
Widget* p = new Widget;
auto sp1 = shared_ptr<Widget>(p);
auto sp2 = shared_ptr<Widget>(p);  // double free！
```

### enable_shared_from_this

对象需要在成员函数里返回自身的 `shared_ptr` 时：
```cpp
class Widget : public std::enable_shared_from_this<Widget> {
public:
    std::shared_ptr<Widget> getPtr() { return shared_from_this(); }
};
// 直接 shared_ptr<Widget>(this) 会创建新控制块 → double free
```

---

## 新手要点（和 C 的区别）

- **C 没有引用计数智能指针**：C 需要手动管理引用计数（如 Linux 内核的 `kref`）。C++ 的 `shared_ptr` 自动管理。
- **代价**：`shared_ptr` 拷贝/析构有原子操作开销（比裸指针慢一个数量级）。别在热路径用。
- **`make_shared` 优先**：一次分配（对象 + 控制块），比 `shared_ptr<T>(new T)` 的两次分配更省且 cache 友好。

---

## HFT 关联

- **热路径绝不用**：`shared_ptr` 的原子计数在多核间引发 cache 行同步，每 tick 路径上拷贝 `shared_ptr` 是性能灾难。
- **对象池/策略共享**：非热路径（对象池、策略对象生命周期管理）可以用，但在线程入口一次性拷贝，热循环内用裸引用。

---

## 自测题

1. `shared_ptr` 的大小是多少？为什么比 `unique_ptr` 大？
2. 用裸指针 `new` 构造两个 `shared_ptr` 会发生什么？
3. `enable_shared_from_this` 解决什么问题？为什么不能直接 `shared_ptr<T>(this)`？
4. `shared_ptr` 拷贝的开销是什么？为什么 HFT 热路径不用它？

<details>
<summary>参考答案</summary>

1. 通常是**两个指针**的大小（64 位平台 16 字节）：一个指向所管理的对象，一个指向**控制块**（control block）。它比 `unique_ptr` 大，是因为引用计数、弱计数、删除器、分配器等元信息必须存在对象之外、被所有 `shared_ptr` 副本共享，所以每个 `shared_ptr` 都要额外携带控制块指针。（控制块里通常还包含指向对象的指针，因此间接层也多一级。）

2. 会形成两个**独立的控制块**，各自认为引用计数为 1，共同拥有同一个裸对象：
```cpp
Widget* w = new Widget;
std::shared_ptr<Widget> p1(w);
std::shared_ptr<Widget> p2(w);   // 灾难：第二个控制块
```
任一计数归零就会 `delete w`，另一个变成悬垂指针，随后二次释放（double free），属于未定义行为。正确做法是直接用 `make_shared`，或只在**一处**用裸指针构造后，其余副本都从已有的 `shared_ptr` 拷贝。

3. `enable_shared_from_this<T>` 让对象内部能安全地产生一个**与已有 `shared_ptr` 共享同一控制块**的 `shared_ptr`（调用 `shared_from_this()`），常用于"把自身注册为回调/异步任务"的场景。不能直接 `shared_ptr<T>(this)`：那会为同一个 `this` 新建一个控制块，与外面已有的 `shared_ptr` 计数互不知情，同样导致双重释放；而且如果对象根本不是由 `shared_ptr` 管理的（栈上对象），这样构造出的 `shared_ptr` 析构时会去 `delete` 一个非堆对象。使用前必须保证对象已被 `shared_ptr` 持有，否则 `shared_from_this()` 抛 `bad_weak_ptr`。

4. 开销有三部分：①每次拷贝/析构都要对引用计数做**原子**增减（跨核的原子 RMW 与内存屏障，比指针赋值慢一个数量级）；②计数与其他 `shared_ptr` 共享同一 cache 行，多线程频繁拷贝会引发 **cache 行在核间来回失效**（false sharing / cache line ping-pong）；③对象与控制块是间接访问，多一次指针跳转。HFT 每 tick 路径对延迟极度敏感，这类不可预测的原子+cache 同步开销会造成延迟尖峰，所以热路径上不用 `shared_ptr`：只在线程入口拷贝一次拿到对象，热循环内改用裸指针或引用，生命周期由外层保证。

</details>

---

## 参考与延伸

- 下一节：[Item 20 weak_ptr](item20-weak-ptr.md)
- 回到：[第 4 章 智能指针](README.md)
