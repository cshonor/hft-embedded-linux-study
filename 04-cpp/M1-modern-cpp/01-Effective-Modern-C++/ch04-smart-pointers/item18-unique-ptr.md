# Item 18：用 std::unique_ptr 管理独占资源

> 第 4 章 智能指针 · Item 18 · 下一节：[Item 19 shared_ptr](item19-shared-ptr.md)

## 为什么要学这个（先建立直觉）

C 里管理堆资源靠纪律：`malloc` 之后配对 `free`，中间有 `goto cleanup`、有异常路径、有早退 `return`——每一条路都得记得释放，漏一条就是泄漏。`unique_ptr` 把这条纪律变成了**类型系统强制的规则**：资源包进对象，对象出作用域自动释放，所有权唯一、拷贝直接被编译器拒绝。

更关键的是 Meyers 强调的卖点：**零开销**。`unique_ptr` 不是"方便但慢一点"的妥协——默认配置下它和裸指针一样大、一样快，是"不用白不用"的纯收益抽象。这一节用实测把这个"零开销"拆开看清楚，包括它在什么情况下**不再零开销**（有状态删除器）。

---

## 这节讲什么

1. 零开销的 `sizeof` 矩阵实测：数组类型、空函数对象、无/有捕获 lambda、函数指针；
2. `unique_ptr` 作为工厂返回类型，以及它到 `shared_ptr` 的单向转换；
3. `get()` / `release()` / `reset()` 所有权三件套的实测语义；
4. 数组特化 `unique_ptr<T[]>`：有下标、无 `*`、不能转 `shared_ptr<T>`；
5. 自定义删除器实战：`FILE*`、`mmap` 区域。全部论断附本机实测（clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`）。

---

## 零开销矩阵：sizeof 实测

```cpp
struct EmptyDeleter { void operator()(int* p) const { delete p; } };
auto stateless = [](int* p) { delete p; };                  // 无捕获
int captured = 7;
auto stateful = [captured](int* p) { (void)captured; delete p; };  // 有捕获
using FnDel = void(*)(FILE*);
// 逐个打印 sizeof(unique_ptr<int>) / unique_ptr<int[]> / unique_ptr<int, EmptyDeleter>
// / unique_ptr<int, decltype(stateless)> / unique_ptr<int, decltype(stateful)>
// / unique_ptr<FILE, decltype(&fclose)> / unique_ptr<FILE, FnDel> / shared_ptr<int>
```

本机实测 clang 23.1.0 输出：

```
sizeof(int*)                                   = 8
sizeof(unique_ptr<int>)                         = 8
sizeof(unique_ptr<Widget>)                      = 8
sizeof(unique_ptr<int[]>)                       = 8
sizeof(unique_ptr<int, EmptyDeleter>)           = 8
sizeof(unique_ptr<int, stateless lambda>)       = 8
sizeof(unique_ptr<int, stateful lambda>)        = 16
sizeof(unique_ptr<FILE, decltype(&fclose)>)     = 16
sizeof(unique_ptr<FILE, FnDel>)                 = 16
sizeof(shared_ptr<int>)                         = 16
```

| 形态 | `sizeof` | 为什么 |
|---|---|---|
| `int*`（对照） | 8 | 裸指针基线 |
| `unique_ptr<T>`（默认删除器） | **8** | `default_delete` 是空类型，EBO 压成 0 |
| `unique_ptr<T[]>` | **8** | 数组特化同样只有一个指针 |
| `unique_ptr<T, 空函数对象>` | **8** | 无状态删除器 → 空基类优化 |
| `unique_ptr<T, 无捕获lambda>` | **8** | 同上 |
| `unique_ptr<T, 有捕获lambda>` | **16** | 删除器有状态，必须存进对象 |
| `unique_ptr<FILE, decltype(&fclose)>` | **16** | 函数指针是**值**，运行期才确定，无法 EBO |
| `shared_ptr<T>`（对照） | 16 | 对象指针 + 控制块指针 |

一句话记忆：**默认配置 8 字节 = 裸指针；删除器"无状态"白送，"有状态"或"函数指针" +8。**函数指针那条最容易踩——`decltype(&fclose)` 推导出来的是 `int(*)(FILE*)`，是个要在运行期读取的**数据成员**，不是可以 EBO 的空类型。想省这 8 字节，把删除器写成空函数对象或无捕获 lambda。

对照删除器自身尺寸（实测）：`sizeof(EmptyDeleter)=1`、`sizeof(无捕获lambda)=1`、`sizeof(有捕获lambda)=4`——空类型单独存在时至少 1 字节，作为基类成员时被压成 0。

---

## 工厂返回类型：所有权随返回值移交

```cpp
std::unique_ptr<Widget> makeWidget(int id) {
    return std::make_unique<Widget>(id);   // 工厂返回 unique_ptr
}
```

这是 C++14 之后工厂函数的标准签名，三重好处：**语义自文档化**（"这个东西归你，别人没有"）、**异常安全**（`make_unique` 内部完成分配+构造+接管，没有裸指针裸露的窗口）、**调用方可以零成本升级**（`unique_ptr` 右值能隐式转 `shared_ptr`）。

### unique_ptr → shared_ptr 是单向免费转换

```cpp
std::shared_ptr<Widget> sp = makeWidget(4);   // OK：右值 unique_ptr 隐式转换
std::cout << sp.use_count();                  // 实测 1
```

关键细节：**转换会把删除器一起搬进控制块**——用自定义删除器的 `unique_ptr<Derived, LogDel>` 转出来的 `shared_ptr`，析构时仍然调 `LogDel`（实测打印 `[LogDel]`）。反过来 `shared_ptr → unique_ptr` **不存在**，因为共享所有权无法降级成独占；从已 move 空的 `unique_ptr` 转换得到的是空 `shared_ptr`（实测 null）。

### 版本差异：`make_unique` 是 C++14

`-std=c++11` 实测报错 `error: no member named 'make_unique' in namespace 'std'`；`-std=c++14/17/20` 全部通过。C++11 项目里自己补一行 `template<class T, class... A> std::unique_ptr<T> make_unique(A&&... a) { return std::unique_ptr<T>(new T(std::forward<A>(a)...)); }`，不要用裸 `new` 凑合。

---

## 所有权三件套：get / release / reset

| 调用 | 语义 | 谁负责释放 | 记忆 |
|---|---|---|---|
| `up.get()` | 借出裸指针，`up` 不变 | 仍是 `up` | **借** |
| `up.release()` | 交出所有权，`up` 置空 | 变成调用方 | **给** |
| `up.reset(p)` | 先释放旧对象，再接管 `p` | `up`（新对象） | **换** |
| `up.reset()` | 释放旧对象，置空 | — | 换（换成空） |

实测（构造/析构带打印）：

```
== get(): 借出，所有权不动 ==
  observe id=1
  still owned: yes
== reset(): 先释放旧的，再接管新的 ==
  ctor #2
  dtor #1          ← reset(new Widget(2)) 时旧的 #1 立刻析构
== release(): 交出所有权，自身置空 ==
  up after release null? yes
  consume id=2
  dtor #2          ← 释放责任已经转给 consume
```

判断准则：**传给"只读一下"的接口用 `get()`**（所有 C API 的观察型函数）；**传给"之后归对方管"的接口用 `release()`**（交给 C 库、塞进 C 结构体、注册到回调表）。`release()` 之后忘了接盘就是泄漏——这是 `unique_ptr` 唯一会主动放弃资源的操作。

---

## 数组特化 `unique_ptr<T[]>`

```cpp
auto arr = std::make_unique<int[]>(4);   // 实测初始值全 0：值初始化
arr[0] = 10; arr[3] = 40;
std::unique_ptr<int[]> a2 = std::move(arr);   // 可移动
std::shared_ptr<int[]> sa = std::move(a2);    // C++17 起可转 shared_ptr<int[]>
```

实测 `arr[0]=10 arr[3]=40`、`a2[0]=10`、`sa.use_count()=1`，尺寸 `sizeof(unique_ptr<int[]>)=8`、`sizeof(unique_ptr<int[8]>)=8`。

**三个必须知道的差异：**

1. **有 `operator[]`，没有 `operator*` / `operator->`**。写 `*arr` 实测报错：

   ```
   error: indirection requires pointer operand ('unique_ptr<int[]>' invalid)
       int x = *arr;
               ^~~~
   ```

2. **不能转成非数组的 `shared_ptr<T>`**。`shared_ptr<int> bad = std::move(arr);` 实测关键诊断行 `note: candidate template ignored: requirement '__compatible_with_v<int[], int>' was not satisfied`，只能转 `shared_ptr<int[]>`（C++17 起的特化）。
3. **`make_unique<T[]>(n)` 是值初始化**（实测全 0），不是 `new T[n]` 的默认初始化；要"不初始化省时间"得等 C++20 的 `make_unique_for_overwrite`。

**和 `std::vector` 怎么选**：默认选 `vector`（有尺寸、有迭代器、能进算法）。`unique_ptr<T[]>` 只在"必须持有裸数组所有权去对接 C API"或"尺寸固定且不想付 vector 三个指针的开销"时用。

---

## 自定义删除器实战

### FILE*：两种写法，两种体积

```cpp
using FilePtr1 = std::unique_ptr<FILE, decltype(&fclose)>;   // 函数指针：16 字节
struct FileCloser { void operator()(FILE* f) const { if (f) fclose(f); } };
using FilePtr2 = std::unique_ptr<FILE, FileCloser>;          // 空函数对象：8 字节

FilePtr1 f1(fopen("/tmp/demo.txt", "w"), &fclose);   // 必须显式传函数指针值
FilePtr2 f2(fopen("/tmp/demo.txt", "r"));            // 删除器默认构造即可
```

实测：`f1` 写入成功、`f2` 读回 `hello unique_ptr`、出作用域打印 `[FileCloser] fclose`；`sizeof(FilePtr1)=16`、`sizeof(FilePtr2)=8`。**同样的功能，写法决定体积**——"零开销"要靠写法人为维持。

### mmap 区域：带状态删除器

```cpp
struct MmapDeleter {
    size_t len;                       // ← 状态：munmap 需要长度
    void operator()(void* p) const { if (p) munmap(p, len); }
};
using MmapPtr = std::unique_ptr<void, MmapDeleter>;

const size_t len = 4096;
void* addr = mmap(nullptr, len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
MmapPtr region(addr, MmapDeleter{len});      // 出作用域自动 munmap
```

实测输出 `sizeof(MmapPtr)=16`（删除器带 `len`，+8）、`region[0]=A`、出作用域打印 `[MmapDeleter] munmap 4096 bytes`。说明删除器"有状态"是正当的：`munmap` 要求长度，而 `unique_ptr` 只存一个指针，状态只能塞进删除器对象。**能无状态就无状态（长度写成编译期常量/模板参数），不能就老实付 8 字节。**对照组：手写 `int* p = new int[10]; delete p;` 实测警告 `warning: 'delete' applied to a pointer that was allocated with 'new[]' [-Wmismatched-new-delete]`——`unique_ptr<T[]>` 从类型上杜绝了这个错配。

---

## 完整可运行例子

```cpp
#include <cstdio>
#include <memory>
#include <iostream>

struct Order {
    int qty;
    explicit Order(int q) : qty(q) { std::cout << "  Order ctor qty=" << qty << "\n"; }
    ~Order() { std::cout << "  Order dtor qty=" << qty << "\n"; }
};
std::unique_ptr<Order> makeOrder(int q) { return std::make_unique<Order>(q); }   // 工厂
void inspect(const Order* o) { std::cout << "  inspect qty=" << o->qty << "\n"; }
struct FileCloser { void operator()(FILE* f) const { if (f) fclose(f); } };

int main() {
    std::cout << std::unitbuf;
    auto up = makeOrder(100);
    inspect(up.get());                 // 借
    up.reset(new Order(200));          // 换（旧的在此析构）
    Order* raw = up.release();         // 给
    std::cout << "  up 已置空? " << (up == nullptr ? "yes" : "no") << "\n";
    delete raw;

    std::shared_ptr<Order> sp = makeOrder(300);
    std::cout << "  use_count=" << sp.use_count() << "\n";

    std::unique_ptr<FILE, FileCloser> fp(fopen("/tmp/emcG_order.txt", "w"));
    fprintf(fp.get(), "qty=100\n");
    std::cout << "  sizeof(fp)=" << sizeof(fp) << "（空删除器 → EBO）\n";

    auto buf = std::make_unique<int[]>(4);
    for (int i = 0; i < 4; ++i) buf[i] = i * 10;
    std::cout << "  buf[3]=" << buf[3] << " sizeof=" << sizeof(buf) << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
  Order ctor qty=100
  inspect qty=100
  Order ctor qty=200
  Order dtor qty=100
  up 已置空? yes
  Order dtor qty=200
  Order ctor qty=300
  use_count=1
  sizeof(fp)=8（空删除器 → EBO）
  buf[3]=30 sizeof=8
  Order dtor qty=300
```

---

## 常见错误（新手踩坑）

1. **同一裸指针构造两个 `unique_ptr`**。`Widget* p = new Widget; unique_ptr<Widget> a(p), b(p);` → 双重释放。ASan（`-fsanitize=address -g`）实测报错 `==94325==ERROR: AddressSanitizer: attempting double-free on 0x602000000990 in thread T0`（栈顶 `default_delete<int>::operator()` ← `unique_ptr::~unique_ptr`）。若析构函数会读成员（如打印 `id`），会先报 `heap-use-after-free`——同一个 bug 的两种现场。**纪律：裸指针交给智能指针后立刻忘掉它；优先 `make_unique`。**
2. **以为函数指针删除器也零开销**。实测 16 字节。要省体积，写成空函数对象或无捕获 lambda。
3. **用裸 `new` 构造而不是 `make_unique`**。能编过，但 `make_unique` 更短、更异常安全，且 C++14 起全标准可用。
4. **移动后还使用源指针**。`std::move` 之后源 `unique_ptr` 是 `nullptr`（实测确认），只剩重新赋值和析构两种合法操作。
5. **`release()` 之后忘了接盘**。这是 `unique_ptr` 唯一会主动放弃资源的接口，用之前先确定谁负责释放。

---

## HFT 关联

- **管 mbuf / fd / 库句柄**：自定义删除器让 `unique_ptr` 管 DPDK `rte_mbuf*`（删除器调 `rte_pktmbuf_free`）、POSIX fd（删除器调 `close`）、库句柄。RAII 保证异常路径、早退路径都不泄漏——比 `goto cleanup` 干净，还零开销。
- **热路径资源的安全网**：热路径上传递所有权用 `unique_ptr` + `std::move`，无引用计数、无原子操作——这点和 `shared_ptr` 形成决定性对比（见 [Item 19](item19-shared-ptr.md)）。
- **工厂函数返回 `unique_ptr`**：`std::unique_ptr<Strategy> make_strategy(Config)` 是策略对象创建的标准签名，接口语义自文档化。
- **定长缓冲用 `unique_ptr<T[]>`**：行情快照的定长档位数组（`unique_ptr<Level[]>`）比 `vector` 少 size/capacity 字段，尺寸不变时零扩容。

---

## 自测题

**1. 默认 `unique_ptr<int>` 多大？凭什么说"零开销"？什么情况下会变成 16 字节？**
<details><summary>答案</summary>
8 字节（实测）。"零开销"指与手写 `new`/`delete` 相比不引入额外内存或时间成本：无引用计数、无控制块，解引用与析构都是内联指针操作，独占所有权靠类型系统（删除拷贝）在编译期表达。变 16 字节的两种情况：删除器是函数指针（实测 `unique_ptr<FILE, decltype(&fclose)>` = 16），或删除器有状态（有捕获 lambda / 带成员的删除器 = 16）。无捕获 lambda 和空函数对象靠 EBO 仍是 8 字节。
</details>

**2. `unique_ptr<FILE, decltype(&fclose)>` 和 `unique_ptr<FILE, 某lambda类型>` 是同一类型吗？能放进同一个 vector 吗？**
<details><summary>答案</summary>
不是同一类型——删除器类型是第二个模板参数，删除器不同则 `unique_ptr` 类型不同，不能互相赋值，也不能进同一个 `vector<unique_ptr<T, D>>`；两个不同的 lambda 类型也互不相同。要进容器，统一用 `unique_ptr<T, std::function<void(T*)>>`（代价是变 32 字节 + 间接调用），或用 `shared_ptr<T>`（删除器类型擦除，不进类型）。
</details>

**3. `.get()` / `.release()` / `.reset()` 各是什么所有权语义？**
<details><summary>答案</summary>
`.get()` 借出裸指针，所有权不动（智能指针仍负责释放）；`.release()` 交出所有权并置空自身（实测 `up == nullptr` 为 true），释放责任转给接收方，忘了 `delete` 就泄漏；`.reset(p)` 先释放当前持有的对象再接管 `p`（实测先打印 `dtor #1`）。记忆法：借 / 给 / 换。
</details>

**4. `unique_ptr` 能转成 `shared_ptr` 吗？反过来呢？转换时删除器怎么处理？**
<details><summary>答案</summary>
`unique_ptr` 右值能隐式转 `shared_ptr`（实测 `use_count=1`），且自定义删除器会被搬进控制块（实测用 `LogDel` 的 `unique_ptr` 转出来的 `shared_ptr` 析构时仍打印 `[LogDel]`）。反过来不行——共享所有权无法降级成独占。数组特化有额外限制：`unique_ptr<T[]>` 只能转 `shared_ptr<T[]>`，转 `shared_ptr<T>` 实测报 `requirement '__compatible_with_v<int[], int>' was not satisfied`。
</details>

**5. `unique_ptr<T[]>` 和 `std::vector<T>` 怎么选？数组特化缺哪些操作？**
<details><summary>答案</summary>
默认 `std::vector<T>`：有尺寸、有迭代器、能进算法。`unique_ptr<T[]>` 只在需要"拥有并移交裸数组所有权"时考虑（对接 C API、定长缓冲）。数组特化有 `operator[]` 但**没有 `operator*` 和 `operator->`**——实测 `*arr` 报 `error: indirection requires pointer operand ('unique_ptr<int[]>' invalid)`。另外 `make_unique<T[]>(n)` 是值初始化（实测全 0），不是 `new T[n]` 的默认初始化。
</details>

**6. `Widget* p = new Widget; unique_ptr<Widget> a(p), b(p);` 会发生什么？ASan 报什么？**
<details><summary>答案</summary>
双重释放（UB）。ASan 实测报 `ERROR: AddressSanitizer: attempting double-free`；若析构函数会读成员（如打印 `id`），会先报 `heap-use-after-free`——同一 bug 的另一种现场。根因是裸指针没有所有权信息。纪律：优先 `make_unique`，让裸指针不具名。
</details>

**7. `std::make_unique` 从哪个标准开始有？C++11 项目里怎么办？**
<details><summary>答案</summary>
C++14。实测 `-std=c++11` 报 `error: no member named 'make_unique' in namespace 'std'`，`-std=c++14/17/20` 全部通过。C++11 项目里自己补 `template<class T, class... A> std::unique_ptr<T> make_unique(A&&... a) { return std::unique_ptr<T>(new T(std::forward<A>(a)...)); }`。不要用裸 `new` 凑合——`make_unique` 消灭的是"`new` 求值与 `unique_ptr` 构造之间抛异常"的泄漏窗口。
</details>

---

## 参考与延伸

- 下一节：[Item 19 shared_ptr](item19-shared-ptr.md)
- 相关：[Item 21 make 函数](item21-make-functions.md)（`make_unique` 相对裸 `new` 的三条理由）
- 回到：[第 4 章 智能指针](README.md)
