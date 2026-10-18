# Item 19：用 std::shared_ptr 管理共享所有权

> 第 4 章 智能指针 · Item 19 · 上一节：[Item 18 unique_ptr](item18-unique-ptr.md)

## 为什么要学这个（先建立直觉）

`unique_ptr` 覆盖"一个资源一个主人"的场景，但现实里总有"多个对象都要用、谁也不知道谁最后用完"的时候——共享配置、共享连接、缓存里的条目。手写所有权协议（"最后离开的人关灯"）在多线程下几乎是必错的，`shared_ptr` 用引用计数把这盏灯自动化了。

但 `shared_ptr` 不是 `unique_ptr` 的"升级版"，是**另一种价格的工具**：体积翻倍、每次拷贝/析构都要原子操作、对象旁边还多一块控制块。这一节的任务就是搞清楚这份价格单的每一项——以及什么时候根本不该付。

---

## 这节讲什么

1. 价格单实测：`sizeof`、分配次数（`make_shared` vs `new`+`shared_ptr`）、原子计数开销；
2. 控制块（control block）里有什么、什么时候创建；
3. 最致命的用法错误：同一裸指针构造两个 `shared_ptr`（含 ASan 抓包现场）；
4. `enable_shared_from_this` 解决什么、为什么 `shared_ptr<T>(this)` 是灾难。

---

## 核心机制：控制块

`shared_ptr` 的共享靠一块**控制块**（control block）实现，里面装着：

- **强引用计数**（有几个 `shared_ptr` 指着对象）
- **弱引用计数**（`weak_ptr` 数量，见 Item 20）
- **删除器与分配器**（类型擦除存放——这是 `shared_ptr` 删除器不进类型的原因，与 `unique_ptr` 相反）
- 对象指针（或对象本身，`make_shared` 时）

```cpp
auto sp = std::make_shared<Widget>();  // 强计数 = 1
auto sp2 = sp;                          // 拷贝 → 强计数 = 2（原子 ++）
// sp2 析构 → 强计数 = 1（原子 --）
// sp 析构  → 强计数 = 0 → 销毁 Widget → 弱计数归零后释放控制块
```

最后一个 `shared_ptr` 析构时对象被销毁——"最后离开的人关灯"，自动且线程安全（**计数本身**线程安全；指向的对象不自动线程安全，别混淆）。

## 价格单实测（clang 23.1.0，arm64）

```
sizeof(shared_ptr<Widget>) = 16    ← 两个指针：对象 + 控制块
sizeof(unique_ptr<Widget>) = 8     ← 对照：裸指针尺寸
```

分配次数（重载全局 `operator new` 计数实测）：

```
make_shared 分配次数: 1        ← 对象 + 控制块一次分配
new + shared_ptr 分配次数: 2   ← 对象一次、控制块一次
```

`make_shared` 把对象和控制块放进**同一块内存**：少一次堆分配、对象与控制块同 cache 行附近、少一次指针跳转。这是"优先 `make_shared`"的实质理由，不只是风格建议。

**`shared_ptr` 三份成本，按频率排序：**

1. **每次拷贝/析构的原子 RMW**：引用计数必须原子增减（多线程共享），一次原子加在竞争下比一个普通 store 慢一个数量级；
2. **cache 行乒乓**：多个核上的 `shared_ptr` 副本共享同一控制块，计数所在的 cache 行在核间来回失效（false sharing 的教科书案例）；
3. **体积与间接**：16 字节/指针 + 控制块另算 + 访问对象要两次解引用。

## 致命错误：一个裸指针，两个控制块

```cpp
Widget* p = new Widget;
auto sp1 = std::shared_ptr<Widget>(p);
auto sp2 = std::shared_ptr<Widget>(p);   // 第二个控制块！
```

`sp1` 和 `sp2` 各自创建控制块、各自计数为 1、互不知情。任一归零就 `delete p`，另一个变悬垂，随后 double free。ASan 抓包现场（clang 23.1.0 + `-fsanitize=address`）：

```
==82166==ERROR: AddressSanitizer: attempting double-free
on 0x614000000040 in thread T0:
```

规则：**裸指针一旦交给 `shared_ptr`，立刻忘掉它**；其余副本永远从已有 `shared_ptr` 拷贝（共享同一控制块）。用 `make_shared` 让裸指针根本没机会具名存在。

## enable_shared_from_this：从对象内部交出自己

场景：对象的成员函数需要返回"指向自己的 `shared_ptr`"（注册回调、投递异步任务）。直接写 `shared_ptr<Widget>(this)` 会**新建控制块**——和外面已有的 `shared_ptr` 互不知情，又是 double free；对象若在栈上则更糟（`delete` 非堆对象）。

正解是继承 `enable_shared_from_this`：

```cpp
class Widget : public std::enable_shared_from_this<Widget> {
public:
    std::shared_ptr<Widget> getPtr() { return shared_from_this(); }
    // 内部存了一个 weak_ptr，shared_from_this() 从已有控制块复制
};

// 前提：对象必须已经由 shared_ptr 管理
auto sp = std::make_shared<Widget>();
auto sp2 = sp->getPtr();   // OK：与 sp 共享控制块

Widget w;
// w.getPtr();            // 抛 std::bad_weak_ptr：没有控制块可共享
```

机制：`enable_shared_from_this` 在对象里埋一个 `weak_ptr`，第一个 `shared_ptr` 接管对象时初始化它；`shared_from_this()` 从这个 `weak_ptr` `lock()` 出共享同一控制块的 `shared_ptr`。所以它**不是凭空造计数**，而是找到已有的计数——前提是已经有一个。

---

## 常见错误（新手踩坑）

1. **同一裸指针构造两个 `shared_ptr`**。如上，ASan 实锤 double-free。最常见的变体：`shared_ptr<T>(this)` 和从 `get()` 拿回来的裸指针再包一层——`shared_ptr<T>(sp.get())` 同样造第二个控制块。
2. **在热路径拷贝 `shared_ptr`**。每次拷贝都是原子 RMW + 潜在 cache 乒乓。热路径传引用或裸指针，只在生命周期边界拷贝。
3. **以为 `shared_ptr` 让对象线程安全**。线程安全的只是引用计数本身；两个线程同时写 `*sp` 指向的对象照样数据竞争。
4. **循环引用泄漏**。A 持 B、B 持 A，计数永远到不了 0，双方都不释放——打破循环用 `weak_ptr`（Item 20）。

---

## 新手要点（和 C 的区别)

- **C 没有引用计数智能指针**。C 里要共享就手写计数协议（Linux 内核的 `kref`、`refcount_t` 就是这套的手写版——`shared_ptr` 本质是把 kref 包进了类型系统）。
- **`make_shared` 优先**：一次分配（实测 1 vs 2）、cache 友好、还顺带消灭"裸指针对外交"的窗口。
- **代价意识**：`shared_ptr` 拷贝/析构带原子操作，比裸指针慢一个数量级。"能用 unique_ptr 就不用 shared_ptr"是 Meyers 的原话立场。

---

## HFT 关联

- **热路径绝不用 `shared_ptr`**：原子计数 + 控制块 cache 行在多核间乒乓，是每 tick 路径上的延迟尖峰发生器。模式：线程入口拷贝一次拿到对象（或干脆裸指针/引用），热循环内零计数操作，生命周期由外层 scope 保证。
- **非热路径的合理用途**：对象池条目、策略配置快照、跨线程异步任务句柄——低频路径上 `shared_ptr` 换来的安全收益远大于微秒级成本。
- **`make_shared` 的副作用要知情**：对象和控制块同块内存，意味着只要还有 `weak_ptr` 活着，**对象占的内存也不能释放**（控制块要等弱计数归零）。大对象 + 长命 `weak_ptr` = 内存滞留，容量敏感场景要评估。

---

## 自测题

1. `shared_ptr` 多大？为什么比 `unique_ptr` 大一倍？控制块里有什么？
2. 实测 `make_shared` 和 `new`+`shared_ptr` 各分配几次堆内存？差别带来什么好处？
3. 同一裸指针构造两个 `shared_ptr` 会发生什么？ASan 报什么？
4. `enable_shared_from_this` 解决什么问题？为什么 `shared_ptr<T>(this)` 不行？调用 `shared_from_this()` 的前提是什么？
5. `shared_ptr` 拷贝的开销由哪几部分组成？为什么多核下更糟？
6. "shared_ptr 是线程安全的"这句话准确吗？

<details>
<summary>参考答案</summary>

1. 16 字节（64 位实测）：一个指针指向对象，一个指向控制块。控制块含强引用计数、弱引用计数、类型擦除的删除器与分配器，以及对象指针（或内嵌对象）。这些元信息必须被所有副本共享，所以每个 `shared_ptr` 都要多背一个控制块指针。

2. `make_shared` 1 次，`new`+`shared_ptr` 2 次（重载全局 `operator new` 计数实测）。好处：少一次堆分配的系统调用/锁竞争；对象与控制块同块内存、局部性好，访问计数和访问对象大概率同 cache 行；还消灭了裸指针外泄的窗口。

3. 形成两个互不知情的控制块，各自计数为 1；任一归零就 `delete` 对象，另一个悬垂，随后双重释放。ASan 实测报 `attempting double-free`。对策：副本永远从已有 `shared_ptr` 拷贝；优先 `make_shared` 让裸指针不具名。

4. 解决"对象成员函数里要交出指向自身的 `shared_ptr`"的问题（注册回调、异步任务）。`shared_ptr<T>(this)` 会为同一对象新建控制块，与外部已有的 `shared_ptr` 计数互相不知道，双重释放；对象在栈上时还会 `delete` 非堆内存。`enable_shared_from_this` 在对象内埋一个 `weak_ptr`，`shared_from_this()` 从它 lock 出共享已有控制块的 `shared_ptr`。前提：对象已被 `shared_ptr` 管理，否则抛 `std::bad_weak_ptr`。

5. 三部分：①拷贝/析构时对计数的原子 RMW（原子加在竞争下比普通 store 慢一个数量级）；②多核下计数所在 cache 行在核间来回失效（控制块被所有副本共享，典型 false sharing）；③16 字节体积 + 访问对象需两次解引用。核越多、拷贝越频繁，cache 乒乓越严重——开销随核数和拷贝频率放大，不是固定成本。

6. 不准确。线程安全的只有**引用计数的增减**：多个线程同时拷贝/析构各自持有的 `shared_ptr` 副本是安全的。但指向的对象没有任何保护——两线程同时写 `*sp` 照样数据竞争；同一个 `shared_ptr` 实例本身被两线程一读一写（如一个 `reset` 一个拷贝）也是竞争。准确说法：控制块线程安全，对象和指针实例不线程安全。

</details>

---

## 参考与延伸

- 下一节：[Item 20 weak_ptr](item20-weak-ptr.md)
- 回到：[第 4 章 智能指针](README.md)
