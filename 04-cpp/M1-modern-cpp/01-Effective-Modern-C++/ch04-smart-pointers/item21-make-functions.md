# Item 21：优先 make_unique / make_shared 而非 new

> 第 4 章 智能指针 · Item 21 · 上一节：[Item 20 weak_ptr](item20-weak-ptr.md)

## 为什么要学这个（先建立直觉）

C 程序员分配内存一步到位：

```c
struct Widget* p = malloc(sizeof(struct Widget));
widget_init(p, args);
free(p);
```

C++ 的 `shared_ptr` 比 C 多了一样东西：**控制块**（强/弱引用计数、删除器等元数据）。直接用 `new` 包装时，对象和控制块是**两次**堆分配：

```cpp
std::shared_ptr<Widget> sp(new Widget(args));  // new Widget 一次 + 控制块一次 = 2 次
auto sp2 = std::make_shared<Widget>(args);     // 对象+控制块合并 = 1 次
```

少一次分配意味着：少一次 malloc 锁竞争、对象和控制块在同一 cache 行、代码还更短。这几乎是白捡的收益——但 `make` 有三个不适用场景（自定义删除器、大对象+长期 weak_ptr、大括号初始化），不知道边界就会从"白捡"变成"白送bug"。

---

## 这节讲什么

优先用 `make_unique`（C++14）/ `make_shared`（C++11）而非直接 `new`：单次分配、异常安全、代码简洁。三个例外：需要自定义删除器、大对象且有长期 `weak_ptr`、需要大括号初始化。

---

## 优势一：单次堆分配（实测）

用全局 `operator new/delete` 计数器观察分配行为，本机实测（clang 23.1.0, `-std=c++17 -Wall`）。`Big` 是 4KB 的大对象：

```cpp
// make_shared：一次分配，对象+控制块同一块内存
auto sp = std::make_shared<Big>();
// new + shared_ptr：两次分配，对象和控制块分开
std::shared_ptr<Big> sp2(new Big);
```

本机实测（用全局 `operator new/delete` 打日志，完整代码与输出见文末）：`make_shared` 一侧只有一行 `new 4120 字节`（4096 对象 + 24 控制块合并）；`new` 一侧是两行 `new 4096 字节` + `new 32 字节`。malloc 次数减半，且对象与控制块相邻，增减引用计数时不会额外 cache miss。

---

## 优势二：异常安全（C++17 前的求值顺序坑）

Meyers 书里的经典例子：

```cpp
void processWidget(std::shared_ptr<Widget> spw, int priority);
int computePriority();   // 可能抛异常

processWidget(std::shared_ptr<Widget>(new Widget), computePriority());
```

C++17 之前，编译器可以按这个顺序执行：① `new Widget`（裸指针产生，没人接管）→ ② `computePriority()`（**抛异常**）→ ③ `shared_ptr` 构造（永不执行）——`new` 出来的 `Widget` 永久泄漏。模拟这个顺序实测（`-fsanitize=address -g`，`ASAN_OPTIONS=detect_leaks=1`）：

```cpp
__attribute__((noinline)) void dangerousOrder() {
    Widget* raw = new Widget;                  // ①
    int prio = computePriority();              // ② 抛异常
    processWidget(std::shared_ptr<Widget>(raw), prio);  // ③ 永不执行
}
```

实测输出：

```
Widget()
computePriority() 抛异常
caught: boom
main 结束（注意没有 ~Widget()）
=================================================================
==86433==ERROR: LeakSanitizer: detected memory leaks
Direct leak of 1 byte(s) in 1 object(s) allocated from:
    #1 in dangerousOrder() emcC_item21_order.cpp:22
SUMMARY: AddressSanitizer: 1 byte(s) leaked in 1 allocation(s).
```

**注意 C++17 已修掉这个坑**：标准规定函数实参的求值**以整个实参为单位**、不允许交错——`shared_ptr<Widget>(new Widget)` 这个实参要么完整构造好，要么还没开始。本机实测（clang 23.1.0, `-std=c++17`）原始写法也**不泄漏**：

```
Widget()
computePriority()
~Widget()          ← shared_ptr 实参已完整构造，栈展开时正常析构
caught: boom
```

但 Meyers 的建议依然成立：**用 `make_shared` 就根本不用关心求值顺序规则**，而且在 C++11/14 模式下编译老代码时这个坑仍然活着。这是"不要依赖你知道编译器怎么做"的典型案例。

---

## 局限一：不能指定自定义删除器

`make_*` 系列没有把删除器传进去的入口：

```cpp
// 自定义删除器必须用 new + 构造函数
std::shared_ptr<FILE> fp(fopen("data.bin", "rb"), &fclose);
// make_shared 做不到这件事——没有接受删除器的重载
```

需要自定义分配器时同理，用 `std::allocate_shared`。

---

## 局限二：大对象 + 长期 weak_ptr → 内存滞留

这是单次分配的**反面**：`make_shared` 把对象和控制块放进同一块内存，而 `weak_ptr` 依赖控制块存活——只要还有一个 `weak_ptr` 活着，**整块内存**（包括早已析构的大对象占的部分）都不能归还。本机实测：

```cpp
struct Big { char data[4096]; };
std::weak_ptr<Big> wp;
{
    auto sp = std::make_shared<Big>();   // vs shared_ptr<Big>(new Big)
    wp = sp;
}   // sp 死：Big 析构了，但内存归还了吗？
// wp 还活着 → 观察 live bytes
```

实测（完整输出见文末"完整可运行例子"）：`make_shared` 版本 `sp` 死后 `live = 4120`——对象已析构，**4120 字节整块滞留**，直到 `wp` 也析构才 `delete 4120`；`new` 版本 `sp` 一死立即 `delete 4096 字节`，只剩 32 字节控制块陪 `wp` 到最后。

对象 4KB、滞留时间 = 最后一个 `weak_ptr` 的寿命。换成 40MB 的帧缓冲 + 一个长期存活的监控 weak_ptr，滞留的就是 40MB。这种场景用 `shared_ptr<Big>(new Big)`。

---

## 局限三：完美转发传不了大括号

`make_*` 的实参是函数参数包，大括号初始化列表**不是有类型的表达式**，推不了导（和 [Item 2](../ch01-deducing-types/item02-auto-type-deduction.md) 的模板推导失败同源）。本机实测报错原文：

```cpp
auto bad = std::make_unique<std::vector<int>>({1, 2, 3});
```

```
error: no matching function for call to 'make_unique'
    auto bad = std::make_unique<std::vector<int>>({1, 2, 3});
               ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
note: candidate template ignored: substitution failure [with _Tp = std::vector<int>]:
      deduced incomplete pack <(no value)> for template parameter '_Args'
```

圆括号则一切正常（实测输出 `10`）：

```cpp
auto v = std::make_unique<std::vector<int>>(10, 0);  // 10 个 0
// 想要大括号语义，绕道：
auto il = {1, 2, 3};
auto v2 = std::make_unique<std::vector<int>>(il);    // OK：il 有类型
// 或直接 new
std::shared_ptr<std::vector<int>> v3(new std::vector<int>{1, 2, 3});
```

另外注意版本差异：`make_unique` 是 **C++14** 才有的（C++11 只有 `make_shared`），本机实测 `-std=c++11` 下报 `error: no member named 'make_unique' in namespace 'std'`。

---

## 完整可运行例子

内存滞留对比的完整实测代码（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
#include <memory>
#include <iostream>
#include <new>

static long g_liveBytes = 0;
void* operator new(std::size_t n) {
    g_liveBytes += n;
    std::cout << "  new " << n << " 字节\n";
    return std::malloc(n);
}
void operator delete(void* p, std::size_t n) noexcept {
    g_liveBytes -= n; std::cout << "  delete " << n << " 字节\n"; std::free(p);
}

struct Big { char data[4096]; };   // 4KB 大对象

int main() {
    std::cout << "== make_shared：对象和控制块同一块 ==\n";
    {
        std::weak_ptr<Big> wp;
        {
            auto sp = std::make_shared<Big>();
            wp = sp;
            std::cout << "  live = " << g_liveBytes << "\n";
        }
        std::cout << "  sp 死后(wp 还活着) live = " << g_liveBytes << "\n";
    }
    std::cout << "  wp 也死后 live = " << g_liveBytes << "\n";

    std::cout << "== new + shared_ptr：对象和控制块分开 ==\n";
    {
        std::weak_ptr<Big> wp;
        {
            std::shared_ptr<Big> sp(new Big);
            wp = sp;
            std::cout << "  live = " << g_liveBytes << "\n";
        }
        std::cout << "  sp 死后(wp 还活着) live = " << g_liveBytes << "\n";
    }
    std::cout << "  wp 也死后 live = " << g_liveBytes << "\n";
}
```

实测输出（前面各节已引用）：

```
== make_shared：对象和控制块同一块 ==
  new 4120 字节
  live = 4120
  sp 死后(wp 还活着) live = 4120
  delete 4120 字节
  wp 也死后 live = 0
== new + shared_ptr：对象和控制块分开 ==
  new 4096 字节
  new 32 字节
  live = 4128
  delete 4096 字节
  sp 死后(wp 还活着) live = 32
  delete 32 字节
  wp 也死后 live = 0
```

---

## 常见错误（新手踩坑）

1. **`make_shared` 里再套 `new`**。`make_shared<Widget>(new Widget)` 把 `Widget*` 当构造实参转发——要么编译失败，要么 `new` 出来的对象没人接管直接泄漏。`make_*` 收的是**构造参数**，不是指针。
2. **大对象 + 长期 `weak_ptr` 还用 `make_shared`**。对象析构后整块内存滞留到最后一个 weak 死亡（本节实测）。修正：`shared_ptr<Big>(new Big)` 分离分配。
3. **以为 `make` 总是异常安全的最优解而忽略版本**。`make_unique` 是 C++14 才有；C++11 工程要么自己写一个简单的（书里有 5 行实现），要么用 `unique_ptr<T>(new T)` 并接受其异常安全分析。
4. **想给 `make_shared` 传大括号**。`{1,2,3}` 推不出类型（实测报错见上）。先把列表绑到 `auto il = {...}` 再传，或用 `new`。

---

## HFT 关联

- **malloc 次数减半 + cache 局部性**：每次堆分配都可能撞内存池锁；`make_shared` 少一次分配，且控制块与对象同块——增减 `use_count()` 后紧接着访问对象数据，大概率同一 cache 行。
- **内存滞留即容量风险**：4KB 实测放大到百万级订单对象，长期 weak 监控会让内存水位居高不下。HFT 里监控/审计引用如果必须长期存在，考虑分离分配或对监控侧改用裸指针+存活期协议。
- **异常安全不依赖编译器版本**：热路径上的 `f(shared_ptr<T>(new T), g())` 写法在 C++11/14 编译模式下仍是泄漏隐患，`make_shared` 一劳永逸。

---

## 自测题

**1. `make_shared` 比 `shared_ptr<T>(new T)` 省在哪？实测分配次数各是多少？**

<details><summary>答案</summary>
省在一次堆分配：`new` 版本 2 次（对象 4096B + 控制块 32B），`make_shared` 1 次（合并 4120B）。附带收益：对象与控制块同 cache 行、代码更短、异常安全。
</details>

**2. `processWidget(shared_ptr<Widget>(new Widget), computePriority())` 在 C++17 前为什么可能泄漏？C++17 改了什么？**

<details><summary>答案</summary>
C++17 前实参求值可交错：`new Widget` → `computePriority()` 抛异常 → `shared_ptr` 构造未执行，裸指针泄漏（实测 ASan 报 Direct leak）。C++17 规定实参求值不可交错，泄漏路径封死；但 C++11/14 模式下坑仍在，`make_shared` 与版本无关地安全。
</details>

**3. 为什么 `make_shared` 不能用于需要自定义删除器的场景？**

<details><summary>答案</summary>
`make_*` 的接口只有"类型 + 构造参数"，没有传入删除器的重载。删除器只能走 `shared_ptr<T>(p, deleter)` 构造函数，例如 `std::shared_ptr<FILE> fp(fopen(...), &fclose);`。
</details>

**4. 大对象 + 长期 `weak_ptr` 为什么不宜用 `make_shared`？实测数据是什么？**

<details><summary>答案</summary>
`make_shared` 把对象与控制块合并分配，而 `weak_ptr` 依赖控制块存活——只要还有 weak 活着，整块内存不能归还。实测：4KB 对象，`make_shared` 版本 sp 死后仍滞留 4120B；`new` 版本 sp 死后立即归还 4096B，只留 32B 控制块。
</details>

**5. `std::make_unique<std::vector<int>>({1,2,3})` 为什么编译失败？怎么绕？**

<details><summary>答案</summary>
`{1,2,3}` 是 braced-init-list，不是有类型的表达式，模板的参数包 `_Args` 无法从中推导（实测报 `deduced incomplete pack <(no value)>`）。绕法：`auto il = {1,2,3}; make_unique<vector<int>>(il)`，或用圆括号 `(10, 0)`，或直接 `new`。
</details>

**6. `make_unique` 是哪个标准版本引入的？C++11 下怎么办？**

<details><summary>答案</summary>
C++14。C++11 只有 `make_shared`（实测 `-std=c++11` 报 `no member named 'make_unique'`）。C++11 下可手写 5 行的 `make_unique`（`return std::unique_ptr<T>(new T(std::forward<Args>(args)...));`），或直接用 `unique_ptr<T>(new T(...))`。
</details>

**7. 下面代码有什么问题？**

```cpp
auto sp = std::make_shared<Widget>(new Widget(42));
```

<details><summary>答案</summary>
`make_shared` 把实参转发给 `Widget` 的构造函数，这里传的是 `Widget*`——若无 `Widget(Widget*)` 构造则编译失败；若恰有则能编译但 `new` 出来的对象没人接管，永久泄漏。正确写法：`std::make_shared<Widget>(42)`，传构造参数而不是指针。
</details>

---

## 参考与延伸

- 上一节：[Item 20 weak_ptr](item20-weak-ptr.md)
- 下一节：[Item 22 Pimpl](item22-pimpl.md)
- 回到：[第 4 章 智能指针](README.md)
