# Item 19：用 std::shared_ptr 管理共享所有权

> 第 4 章 智能指针 · Item 19 · 上一节：[Item 18 unique_ptr](item18-unique-ptr.md) · 下一节：[Item 20 weak_ptr](item20-weak-ptr.md)

## 为什么要学这个（先建立直觉）

`unique_ptr` 覆盖"一个资源一个主人"的场景，但现实里总有"多个对象都要用、谁也不知道谁最后用完"的时候——共享配置、共享连接、缓存里的条目。手写所有权协议（"最后离开的人关灯"）在多线程下几乎是必错的，`shared_ptr` 用引用计数把这盏灯自动化了。

但 `shared_ptr` 不是 `unique_ptr` 的"升级版"，是**另一种价格的工具**：体积翻倍、每次拷贝/析构都要原子操作、对象旁边还多一块控制块。这一节的任务就是搞清楚这份价格单的每一项——以及什么时候根本不该付。

---

## 这节讲什么

1. 控制块里有什么、什么时候创建、分配几次（呼应 [Item 21 make 函数](item21-make-functions.md)）；
2. **引用计数的线程安全 ≠ 所指对象的线程安全**（实测区分）；
3. 循环引用：LeakSanitizer 抓泄漏现场，`weak_ptr` 破环（详见 [Item 20](item20-weak-ptr.md)）；
4. `enable_shared_from_this` 机制，以及 `shared_ptr<T>(this)` 为什么是灾难；
5. 性能实测：原子计数单价、多核 cache 行乒乓、`make_shared` 的内存滞留副作用。全部论断附本机实测（clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`）。

---

## 控制块：共享的物理载体

`shared_ptr` 的共享靠一块**控制块**（control block）实现，里面装着：强引用计数、弱引用计数（归零才释放控制块本身）、**类型擦除**的删除器与分配器（这就是 `shared_ptr` 删除器不进类型的原因，与 `unique_ptr` 相反），以及对象指针（`make_shared` 时对象内嵌在控制块后面）。

```cpp
auto sp = std::make_shared<Widget>();  // 强计数 = 1
auto sp2 = sp;                          // 拷贝 → 强计数 = 2（原子 ++）
// sp 析构  → 强计数 = 0 → 销毁 Widget → 弱计数归零后释放控制块
```

**控制块何时创建**（决定了所有坑）：`make_shared<T>()` 新建（对象内嵌，一次分配）；`shared_ptr<T>(new T)` 新建（对象独立，两次分配）；从已有 `shared_ptr`/`weak_ptr` 拷贝**不新建**；`shared_ptr<T>(rawPtr)` **一定新建**——哪怕这个裸指针已经被人管着。最后这条就是所有 double-free 的根源。

---

## 分配次数实测（重载全局 operator new 计数）

```
sizeof(shared_ptr<Widget>) = 16    ← 两个指针：对象 + 控制块
sizeof(unique_ptr<Widget>) = 8     ← 对照：裸指针尺寸
```

实测（重载 `operator new` / `operator delete` 统计次数与块大小，`Widget` 为 16 字节）：

```
make_shared<Widget>()            分配次数 = 1   块大小: 40
new Widget + shared_ptr          分配次数 = 2   块大小: 16 32
make_unique<Widget>()            分配次数 = 1   块大小: 16
make_shared +2copy +weak_ptr     分配次数 = 1   块大小: 40
shared_ptr(new W, custom del)    分配次数 = 2   块大小: 16 32
```

三个可以直接读出来的结论：①**`make_shared` 一次分配，`new`+`shared_ptr` 两次**——和 [Item 21](item21-make-functions.md) 的结论一致，这里是计数实锤；②**控制块本身 32 字节**（第二块），`make_shared` 的合并块是 40 = 16（对象）+ 24（控制块），内嵌形态省掉了一份对象指针；③**拷贝和 `weak_ptr` 都不分配**——它们只动计数。

`make_shared` 的好处不只是"少一次分配"：对象和控制块**同块内存**，访问计数与访问对象大概率落在同一 cache 行；同时消灭了"裸指针外泄"的窗口。

### 副作用：长命 weak_ptr 会滞留对象内存

`make_shared` 的合并分配是有代价的——控制块和对象同生共死。实测（统计未释放字节数）：

```
make_shared 后              live = 88 bytes
  ~Big（对象已析构）
所有 shared_ptr 销毁后      live = 88 bytes   wp.expired()=true
weak_ptr 也 reset 后        live = 0 bytes
```

`~Big` 已经跑了（对象**析构**了），但那 88 字节**没有还给分配器**，因为只要还有一个 `weak_ptr` 活着，控制块就不能释放，而对象跟控制块在同一块内存里。**大对象 + 长命 `weak_ptr` = 内存滞留**——这是 `make_shared` 唯一的已知劣势，`new`+`shared_ptr` 版本则会在强计数归零时立刻归还对象内存。

---

## 线程安全：计数安全 ≠ 对象安全

这是最容易被一句话带偏的地方。实测三组：

**A. 引用计数本身**：8 个线程各持 `sp` 的一份副本，各做 10 万次 `auto local = sp;`（原子 ++ 后立即原子 --）。实测输出 `Res ctor` → `main 里 use_count = 1` → `Res dtor`，**只析构一次**，ASan 无报告。计数精确归位。

**B. 所指对象**：两线程各做 20 万次 `++(*sp)`，实测输出 `*sp = 255164`（期望 400000）。**`shared_ptr` 完全没有保护对象**——要保护得自己加锁或用 `std::atomic`。

**C. 同一个 `shared_ptr` 实例被两线程一读一写**（一个 `reset()`、一个拷贝）——同样是数据竞争。

| 谁 | 线程安全吗 |
|---|---|
| 控制块里的引用计数 | ✅ 原子增减，多线程同时拷贝/析构各自副本安全 |
| 同一个 `shared_ptr` **实例** | ❌ 一线程 `reset` 另一线程读 = 竞争 |
| 所指对象 `*sp` | ❌ 与普通对象无异，需自己加锁 |

准确说法：**控制块线程安全，指针实例和所指对象不线程安全。**

---

## 循环引用：LeakSanitizer 现场

```cpp
struct A { std::shared_ptr<B> b; ~A(){ std::cout << "  A dtor\n"; } };
struct B { std::shared_ptr<A> a; ~B(){ std::cout << "  B dtor\n"; } };  // ← 强引用成环
auto a = std::make_shared<A>();
auto b = std::make_shared<B>();
a->b = b;  b->a = a;      // 离开作用域
```

实测输出（**没有任何 dtor 行**——计数永远到不了 0）：

```
  离开作用域前 use_count: a=2 b=2
  离开作用域后（↑ 若没有 A dtor/B dtor 输出 = 泄漏）
```

ASan 的 LeakSanitizer（`ASAN_OPTIONS=detect_leaks=1`）实测报告：

```
==94780==ERROR: LeakSanitizer: detected memory leaks
Indirect leak of 40 byte(s) in 1 object(s) allocated from:
    #8 std::__1::shared_ptr<B> std::__1::make_shared... shared_ptr.h:668
SUMMARY: AddressSanitizer: 80 byte(s) leaked in 2 allocation(s).
```

把 `B` 侧的 `shared_ptr<A>` 换成 `weak_ptr<A>` 再跑，实测输出立刻正常：

```
  离开作用域前 use_count: a=1 b=2     ← a 的强计数只剩 1（weak_ptr 不增计数）
  WA dtor
  WB dtor
```

破环的通用规则：**父子关系里，子指向父用 `weak_ptr`**（详见 [Item 20 weak_ptr](item20-weak-ptr.md)）。

---

## enable_shared_from_this：从对象内部交出自己

场景：对象的成员函数需要返回"指向自己的 `shared_ptr`"（注册回调、投递异步任务）。直接写 `shared_ptr<Widget>(this)` 会**新建控制块**。

实测用 `enable_shared_from_this`：

```cpp
class Widget : public std::enable_shared_from_this<Widget> {
public:
    std::shared_ptr<Widget> selfShared() { return shared_from_this(); }
    std::shared_ptr<Widget> selfWrong()  { return std::shared_ptr<Widget>(this); } // 灾难
};
```

实测输出：

```
== 正解：对象已被 shared_ptr 管理 ==
  use_count=2  同一控制块? yes
== 错误：栈对象上调 shared_from_this() ==
  caught std::bad_weak_ptr: bad_weak_ptr
== 灾难：shared_ptr<Widget>(this) ==
  use_count sp=1 sp2=1（互不知情，都是 1）
```

机制：`enable_shared_from_this` 在对象里埋一个 `weak_ptr`，第一个 `shared_ptr` 接管对象时初始化它；`shared_from_this()` 从这个 `weak_ptr` `lock()` 出共享同一控制块的 `shared_ptr`。所以它**不是凭空造计数**，而是找到已有的计数——前提是已经有一个（否则抛 `std::bad_weak_ptr`，实测捕获成功）。

`selfWrong()` 在 `-fsanitize=address` 下实测报 `==94815==ERROR: AddressSanitizer: attempting free on address which was not malloc()-ed: 0x604000000468`：因为 `make_shared` 把对象放在合并块中间，新控制块拿到的是**内部指针**，最终 `delete` 一个非 malloc 返回的地址。栈上对象用这招则是 `delete` 非堆内存——同样是 UB。

---

## 性能：原子计数与 cache 行乒乓

实测（`-O2`，20,000,000 次循环，每次取指针并累加到 `volatile` 防止优化掉）：

```
裸指针       20000000 次: 17.3061 ms  (0.865306 ns/次)
unique_ptr   20000000 次: 13.8169 ms  (0.690844 ns/次)
shared_ptr   20000000 次: 280.235 ms  (14.0118 ns/次)
1 线程 x 2e6 shared_ptr 拷贝: 26.367 ms
4 线程 x 2e6 shared_ptr 拷贝: 392.461 ms
```

**一次"拷贝 + 析构"约 14 ns，是裸指针的 20 倍**——不是"慢一点"，是数量级差。拆开看：①**原子 RMW**（x86 `lock xadd`、ARM LL/SC 循环，无竞争也这个量级）；②**cache 行乒乓**——4 线程做 4 倍工作却花 **14.9 倍**时间（26.4 ms → 392.5 ms），单次成本从 ~13 ns 涨到 ~49 ns，控制块是所有副本的汇聚点，false sharing 的教科书案例；③**体积与间接**——16 字节/指针 + 控制块另算 24–32 字节 + 访问对象两次解引用。

**一句话对照 intrusive 指针**（`boost::intrusive_ptr`、内核 `kref`）：计数放在**对象内部**而非旁挂控制块，24–32 字节的控制块和第二次解引用全省了，代价是类型必须自带计数字段、且不支持 `weak_ptr`——`shared_ptr` 是"通用但贵"，intrusive 是"更快但要改造类型"。

---

## 完整可运行例子

```cpp
#include <memory>
#include <iostream>

struct Node {
    std::string name;
    std::shared_ptr<Node> child;      // 强引用
    std::weak_ptr<Node>   parent;     // 弱引用：打破环
    explicit Node(std::string n) : name(std::move(n)) { std::cout << "  ctor " << name << "\n"; }
    ~Node() { std::cout << "  dtor " << name << "\n"; }
};
class Session : public std::enable_shared_from_this<Session> {
public: std::shared_ptr<Session> self() { return shared_from_this(); } };

int main() {
    std::cout << std::unitbuf;
    auto root = std::make_shared<Node>("root");
    auto leaf = std::make_shared<Node>("leaf");
    root->child = leaf;  leaf->parent = root;
    std::cout << "  root.use_count=" << root.use_count()
              << "  leaf.use_count=" << leaf.use_count() << "\n";

    std::weak_ptr<Node> w = leaf;                       // 不增强计数
    std::cout << "  leaf.use_count=" << leaf.use_count()
              << "  w.expired()=" << (w.expired() ? "true" : "false") << "\n";
    if (auto locked = w.lock()) std::cout << "  lock() 拿到 " << locked->name << "\n";

    auto s = std::make_shared<Session>();
    auto s2 = s->self();
    std::cout << "  同一控制块? " << (s.get() == s2.get() ? "yes" : "no")
              << "  use_count=" << s.use_count() << "\n";
    std::cout << "  sizeof(shared_ptr<Node>)=" << sizeof(std::shared_ptr<Node>)
              << "  sizeof(unique_ptr<Node>)=" << sizeof(std::unique_ptr<Node>) << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
  ctor root
  ctor leaf
  root.use_count=1  leaf.use_count=2
  leaf.use_count=2  w.expired()=false
  lock() 拿到 leaf
  同一控制块? yes  use_count=2
  sizeof(shared_ptr<Node>)=16  sizeof(unique_ptr<Node>)=8
  dtor root
  dtor leaf
```

注意 `root.use_count=1` 而 `leaf.use_count=2`：`leaf` 被 `root->child` 强持有，而 `root` 只被 `leaf->parent` **弱**持有——环被打破了，两个对象都能正常析构。

---

## 常见错误（新手踩坑）

1. **同一裸指针构造两个 `shared_ptr`**。形成两个互不知情的控制块，各自计数为 1，任一归零就 `delete` 对象，另一个悬垂。变体包括 `shared_ptr<T>(this)` 和 `shared_ptr<T>(sp.get())`。对策：副本永远从已有 `shared_ptr` 拷贝；优先 `make_shared`。
2. **在热路径拷贝 `shared_ptr`**。实测 14 ns/次，多核争用更贵。热路径传引用或裸指针，只在生命周期边界拷贝。
3. **以为 `shared_ptr` 让对象线程安全**。线程安全的只有引用计数本身；实测两线程各 20 万次 `++(*sp)` 得到 255164 而不是 400000。
4. **循环引用泄漏**。A 持 B、B 持 A，Lsan 实测报 `80 byte(s) leaked in 2 allocation(s)`。打破循环用 `weak_ptr`。
5. **忽略 `make_shared` 的滞留副作用**。还有 `weak_ptr` 活着时，对象占的内存不会归还（实测 88 bytes 一直挂着）。大对象场景要评估。

---

## HFT 关联

- **热路径绝不用 `shared_ptr`**：原子计数 + 控制块 cache 行在多核间乒乓（实测 4 线程争用把单次成本从 13 ns 抬到 49 ns），是每 tick 路径上的延迟尖峰发生器。模式：线程入口拷贝一次拿到对象（或干脆裸指针/引用），热循环内零计数操作，生命周期由外层 scope 保证。
- **非热路径的合理用途**：对象池条目、策略配置快照、跨线程异步任务句柄——低频路径上 `shared_ptr` 换来的安全收益远大于微秒级成本。
- **行情网关的订阅关系**：Subscriber 持有 `shared_ptr<Session>`、Session 持有 `weak_ptr<Subscriber>`，既保证回调时对象还活着，又不形成环——这是本节"子指父用 weak"规则的真实落点。
- **大对象慎用 `make_shared` + 长命 weak_ptr**：订单簿快照这类几十 KB 的对象，若缓存持 `weak_ptr`，强计数归零后内存仍滞留，容量敏感场景改用 `shared_ptr<T>(new T)`。

---

## 自测题

**1. `shared_ptr` 多大？控制块里有什么？控制块什么时候创建？**
<details><summary>答案</summary>
16 字节（实测）：一个指针指向对象，一个指向控制块。控制块含强/弱引用计数、类型擦除的删除器与分配器、对象指针（`make_shared` 时对象内嵌，实测合并块 40 = 16 对象 + 24 控制块；独立控制块实测 32 字节）。创建时机：`make_shared` / `shared_ptr<T>(new T)` / `shared_ptr<T>(裸指针)` 新建；从已有 `shared_ptr`、`weak_ptr` 拷贝不新建。
</details>

**2. 实测 `make_shared` 和 `new`+`shared_ptr` 各分配几次？差别带来什么好处和什么副作用？**
<details><summary>答案</summary>
`make_shared` 1 次（实测块 40 字节），`new`+`shared_ptr` 2 次（实测 16 + 32）。好处：少一次堆分配、局部性好、消灭裸指针外泄窗口。副作用：对象与控制块同生共死，只要还有 `weak_ptr` 活着，对象内存就不能归还——实测 `~Big` 已执行但 88 字节仍挂着，直到 `wp.reset()` 才归零。
</details>

**3. "shared_ptr 是线程安全的"这句话准确吗？请用实测说明。**
<details><summary>答案</summary>
不准确。线程安全的只有**引用计数的增减**：实测 8 线程各 10 万次拷贝/析构副本，最终 `use_count` 精确为 1、对象只析构一次。但所指对象没有保护——实测两线程各 20 万次 `++(*sp)` 得到 255164 而不是 400000；同一个 `shared_ptr` 实例被一线程 `reset`、另一线程拷贝也是竞争。准确说法：控制块线程安全，指针实例和所指对象不线程安全。
</details>

**4. 循环引用为什么泄漏？Lsan 报什么？怎么破？**
<details><summary>答案</summary>
A 持 `shared_ptr<B>`、B 持 `shared_ptr<A>`，互相把对方计数顶到 ≥1，谁都归不了零。实测离开作用域后没有任何 dtor 输出，Lsan 报 `ERROR: LeakSanitizer: detected memory leaks` + `SUMMARY: AddressSanitizer: 80 byte(s) leaked in 2 allocation(s)`。破法：一侧改成 `weak_ptr`（通常"子指父"），实测改完 `use_count` 从 2 降到 1，两个 dtor 正常打印。
</details>

**5. `enable_shared_from_this` 解决什么问题？为什么 `shared_ptr<T>(this)` 不行？调用 `shared_from_this()` 的前提是什么？**
<details><summary>答案</summary>
解决"对象成员函数里要交出指向自身的 `shared_ptr`"（注册回调、异步任务）。`shared_ptr<T>(this)` 会新建控制块：实测 `use_count sp=1 sp2=1` 互不知情，ASan 下报 `attempting free on address which was not malloc()-ed`（因为 `make_shared` 的对象在合并块中间）。`enable_shared_from_this` 在对象内埋 `weak_ptr`，`shared_from_this()` 从它 lock 出共享已有控制块的 `shared_ptr`。前提：对象已被某个 `shared_ptr` 管理，否则实测抛 `std::bad_weak_ptr`。
</details>

**6. `shared_ptr` 拷贝的开销由哪几部分组成？为什么核越多越糟？**
<details><summary>答案</summary>
三部分：①对计数的原子 RMW（实测 14 ns/次，裸指针 0.87 ns、unique_ptr 0.69 ns）；②多核下计数所在 cache 行在核间来回失效——实测 4 线程做 4 倍工作却花 14.9 倍时间（26.4 ms → 392.5 ms），单次成本从 ~13 ns 涨到 ~49 ns；③16 字节体积 + 两次解引用。开销随核数和拷贝频率**放大**，不是固定成本。
</details>

**7. 和 intrusive 指针（引用计数放在对象里）相比，`shared_ptr` 贵在哪、赢在哪？**
<details><summary>答案</summary>
贵在：旁挂控制块（实测 24–32 字节）额外分配、可能两次 cache miss、指针本身 16 字节。赢在：类型无侵入（不需要自带计数字段）、支持不完整类型、支持 `weak_ptr`、删除器类型擦除不进类型。intrusive 更快但要改造类型且不支持 `weak_ptr`。
</details>

---

## 参考与延伸

- 上一节：[Item 18 unique_ptr](item18-unique-ptr.md)
- 下一节：[Item 20 weak_ptr](item20-weak-ptr.md)
- 相关：[Item 21 make 函数](item21-make-functions.md)（`make_shared` 相对裸 `new` 的三条理由）
- 回到：[第 4 章 智能指针](README.md)
