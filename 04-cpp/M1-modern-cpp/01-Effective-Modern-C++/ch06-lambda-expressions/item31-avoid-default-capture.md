# Item 31：避免默认捕获模式（[=] / [&]）

> 第 6 章 Lambda 表达式 · Item 31 · 下一节：[Item 32 初始化捕获](item32-init-capture.md)

## 为什么要学这个（先建立直觉）

lambda 的捕获列表是它和 C 的「函数指针 + `void*` userdata」最本质的区别：编译器帮你把上下文打包进闭包。但 `[=]` 和 `[&]` 这两个「默认捕获模式」相当于对编译器说「我用了什么你就给我捕获什么」——**你放弃了对捕获清单的审查权**。问题在闭包**逃出作用域**时才爆发：`[&]` 捕获了局部变量的 lambda 被返回、被塞进任务队列、被注册成回调，等它真正执行时，当初捕获的局部变量早就随栈帧销毁了。引用不会延长对象的生命周期——它只是一个地址，对象死了，地址还在。

更隐蔽的是 `[=]`：在成员函数里写 `[=]`，字面上是「按值捕获我用到的一切」，但成员变量根本**不能被直接捕获**——闭包实际捕获的是 `this` 指针，每次访问成员都走 `this->data`。你以为拿到了一份**拷贝**，实际拿到的是一根**指针**。

---

## 这节讲什么

1. `[&]` 按引用捕获的**悬垂实测**：局部变量销毁后调用闭包，普通编译读垃圾值，ASan 抓 `stack-use-after-scope`；
2. `[=]` 的**假象**：捕获成员变量实际是捕获 `this` 指针，ASan 帧信息指出「访问落在已析构的 `w` 里」；
3. **static 局部变量不被捕获**（`[&]` 也捕获不到），用地址和 `sizeof` 实测证明；C++20 的 `[=, this]` 显式化（编译器警告原文实测）。

---

## [&] 悬垂：闭包比局部变量活得久

经典反面教材——返回一个按引用捕获局部变量的 lambda：

```cpp
#include <functional>
#include <iostream>

std::function<int()> getCounter() {
    int count = 100;                    // 局部变量
    return [&] { return ++count; };     // 按引用捕获 count —— return 后 count 销毁
}

int main() {
    auto f = getCounter();   // 栈帧已销毁，count 不复存在，闭包里只剩悬垂地址
    std::cout << "f() = " << f() << "\n";
    std::cout << "f() = " << f() << "\n";
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`），正确输出应该是 `101 102`，实际却是：

```
f() = 2
f() = 2
```

栈内存被后续调用覆写，读到的是垃圾值（加 `-fsanitize=address -g` 再跑垃圾值变成 `1807838081`，但 **ASan 没有报错**）。`std::function` 的类型擦除调用链让 use-after-scope 检测漏了过去——sanitizer 不一定救你，程序甚至可能「看起来能跑」。

不经过 `std::function` 直接返回 lambda 时，编译器倒是会警觉（本机实测警告原文）：

```
warning: address of stack memory associated with local variable
         'count' returned [-Wreturn-stack-address]
note: implicitly captured by reference due to use here
```

编译器能看穿直接返回，但看不穿「存进 `std::function` / 任务队列 / 回调注册表」的间接逃逸——真实代码里闭包几乎都是间接逃逸的。**不要指望警告救你，要靠纪律：显式列出捕获变量。**

---

## [=] 的假象：捕获的是 this 指针

成员变量不能单独出现在捕获列表里，`[=]` 用到成员时实际捕获 `this`：

```cpp
#include <functional>
#include <iostream>

class Widget {
    int data;
public:
    explicit Widget(int d) : data(d) {}
    std::function<int()> makeCallback() {
        return [=] { return data; };   // 看着像"按值捕获 data"
                                       // 实际等价于 [this]{ return this->data; }
    }
};

int main() {
    std::function<int()> cb;
    {
        Widget w{42};
        cb = w.makeCallback();
        std::cout << "对象存活时 cb() = " << cb() << "\n";
    }   // w 析构，闭包里的 this 悬垂
    std::cout << "对象销毁后 cb() = " << cb() << "\n";  // UB
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）普通编译的输出是**最阴险的那种结果**：

```
对象存活时 cb() = 42
对象销毁后 cb() = 42
```

两次都是 42——栈内存恰好没被覆写，UB「看起来完全正常」，能在测试环境潜伏很久。同一份代码用 `-fsanitize=address -g` 编译，ASan 立刻现形（实测输出节选）：

```
==85017==ERROR: AddressSanitizer: stack-use-after-scope on address 0x00016fae6e00
READ of size 4 at 0x00016fae6e00 thread T0
    #0 Widget::makeCallback()::'lambda'()::operator()() const
    [96, 100) 'w' (line 24) <== Memory access at offset 96 is inside this variable
```

最后一行是铁证：闭包里的「`data`」访问落在了**已析构的 `w`** 的内存里——`[=]` 根本没有拷贝 `data`，它只是存了 `&w`。

### 正确做法

```cpp
// 方案一（C++11 起通用）：先拷贝到局部变量，再按值捕获局部变量
std::function<int()> makeCallback() {
    int local_data = data;                       // 拷贝发生在对象存活时
    return [local_data] { return local_data; };  // 闭包持有真正的副本
}
// 方案二（C++14 起）：init capture，一步到位（Item 32 详讲）
std::function<int()> makeCallback() {
    return [d = data] { return d; };
}
// 方案三（C++17 起）：[*this] 按值捕获整个对象的副本
std::function<int()> makeCallback() {
    return [*this] { return data; };   // 闭包里存了一个 Widget 拷贝
}
```

三种方案的共同点是**拷贝发生在对象还活着的时候**，闭包此后与原对象再无瓜葛。方案三拷的是**整个对象**，对象大时方案二（只拷需要的成员）更省。

---

## static 局部变量不被捕获

`[&]` 的语义是「按引用捕获我用到的**自动存储期**变量」。`static` 局部变量不在栈上，根本不需要捕获——lambda 里写它的名字，用的就是静态区里那一份：

```cpp
#include <iostream>

void demonstrate() {
    static int s_count = 0;    // 静态区，不在栈上
    int local = 0;             // 普通局部变量
    auto lam = [&] {
        ++s_count;             // 没被捕获！直接用静态区的那份
        ++local;               // 这个才被 [&] 捕获
        std::cout << "lambda 内  &s_count = " << static_cast<void*>(&s_count)
                  << " 值 = " << s_count << "\n";
    };
    std::cout << "lambda 外  &s_count = " << static_cast<void*>(&s_count)
              << " 值 = " << s_count << "\n";
    lam();  lam();
    std::cout << "两次调用后  s_count = " << s_count
              << "  local = " << local << "\n";
    std::cout << "sizeof(lambda) = " << sizeof(lam)
              << "（只装了一个引用：local）\n";
}

int main() { demonstrate(); }
```

本机实测（clang 23.1.0）输出：

```
lambda 外  &s_count = 0x102ce0090 值 = 0
lambda 内  &s_count = 0x102ce0090 值 = 1
lambda 内  &s_count = 0x102ce0090 值 = 2
两次调用后  s_count = 2  local = 2
sizeof(lambda) = 8（只装了一个引用：local）
```

三个证据：lambda 内外 `&s_count` 完全相同，是同一份实体；`sizeof(lambda) == 8` 恰好一个指针大小，闭包里只装了 `local` 的引用；static 的递增全局可见。推论：**`[&]` 不会「冻结」static 变量的状态，也不让它进闭包**——它永远不悬垂，但多线程共享时要自行同步。

---

## C++20 的 [=, this] 显式化

C++20 弃用了 `[=]` 对 `this` 的**隐式**捕获，要求你写明。本机实测（clang 23.1.0，`-std=c++20`）：

```cpp
return [=] { return data; };        // C++20 起：弃用警告
return [=, this] { return data; };  // C++20：显式声明捕获 this
```

编译警告原文：

```
warning: implicit capture of 'this' with a capture default of '='
         is deprecated [-Wdeprecated-this-capture]
note: add an explicit capture of 'this' to capture '*this' by reference
```

措辞是 deprecated（弃用）而不是 error——老代码还能编译，但新标准逼你承认「我知道我捕获的是指针」。反过来 `[=, this]` 在 `-std=c++17` 下会得到 `... is a C++20 extension` 的扩展警告（clang 兼容放行）。一句话：**C++20 里要么 `[=, this]` 明示按指针捕获，要么 `[*this]` 按值捕获对象，别再用裸 `[=]` 糊过去。**

---

## 完整可运行例子

安全写法汇总——临时对象造出的过滤器、逃逸出作用域的计数器，全部显式按值捕获：
```cpp
#include <functional>
#include <iostream>

class Engine {
    int threshold;
public:
    explicit Engine(int t) : threshold(t) {}
    std::function<bool(int)> makeFilter() const {
        int t = threshold;                        // 拷贝发生在对象存活时
        return [t](int price) { return price > t; };
    }
};

std::function<int()> getCounter() {
    int count = 0;
    return [count]() mutable { return ++count; };  // 按值捕获 + mutable 允许改副本
}

int main() {
    std::cout << std::boolalpha;
    auto filter = Engine{100}.makeFilter();   // 临时 Engine 语句结束即析构
    std::cout << "filter(150) = " << filter(150) << "\n";  // 闭包持有 t 副本，安全
    std::cout << "filter(50)  = " << filter(50) << "\n";
    auto c = getCounter();
    std::cout << c() << " " << c() << " " << c() << "\n";  // 各改各的副本
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）输出：

```
filter(150) = true
filter(50)  = false
1 2 3
```

临时 `Engine` 析构后过滤器依然正确——闭包持有的 `t` 是对象存活时拷出的副本。

---

## 常见错误（新手踩坑）

1. **成员函数里用 `[=]` 以为按值捕获了成员**。实际捕获的是 `this` 指针，对象析构后闭包悬垂（上文 ASan 实测）。修正：`[d = data]`（C++14）或先拷到局部再 `[d]`，或 `[*this]`（C++17）。
2. **`[&]` 捕获局部变量后让闭包逃逸**（返回、塞进队列、`std::async`）。局部变量销毁后引用悬垂，可能读到垃圾值也不一定被 ASan 抓到。修正：逃逸的闭包一律显式按值捕获。
3. **以为 `[&]` 会捕获 static 局部变量**。static 变量不进闭包，lambda 用的是静态区那一份（地址实测相同），多线程共享时要自己加同步。

---

## HFT 关联

- **策略回调的 `this` 悬垂**：`engine.on_tick([=](const Tick& t){ ... })` 写在成员函数里，策略对象析构后引擎仍持有闭包——热重载策略、盘中切换实例时真实会炸。评审时把成员函数里的 `[=]` 当红旗 grep：`grep -n 'return \[=\]'`。
- **UB「看起来正常」最致命**：本文实测析构后仍打印 42——性能测试环境一切正常、生产环境偶发错乱。逃逸闭包的捕获清单必须人工评审，不能靠测试通过背书。
- **异步任务显式按值捕获**：`std::async([config](const Tick& t){ ... })` 捕获副本而非引用——配合 [Item 35](../ch07-concurrency-api/item35-async-vs-thread.md) 的任务制并发，每个任务自含全部上下文，无悬垂可能。

---

## 自测题

**1. `[=]` 在成员函数里捕获成员变量时，实际捕获的是什么？为什么这是隐患？**
<details><summary>答案</summary>
实际捕获的是 `this` 指针——成员变量不能单独被捕获，`[=]` 用到成员时等价于 `[this]`，闭包里每次访问都是 `this->data`。隐患有二：一是对象析构后 `this` 悬垂，闭包读写成员是 UB（本文 ASan 实测：访问落在已析构对象 `w` 的内存里）；二是「按值捕获」的字面语义误导读者以为拿到了副本，实际拿到的是裸指针。修正：`[d = data]` 或 `[*this]`。
</details>

**2. 为什么 `[&]` 悬垂有时 ASan 抓不到、普通运行甚至结果「正确」？**
<details><summary>答案</summary>
悬垂引用读的是已释放的栈内存，只要那段内存还没被覆写，读到的就是旧值——UB 不保证报错，只保证「行为无定义」。本文实测中普通编译的 `[=]`-this 版本析构后仍打印 42；而 `std::function` 类型擦除的间接调用链又让 ASan 的 use-after-scope 插桩失效，垃圾值照打不误。结论：sanitizer 是辅助，不是保险；逃逸闭包靠显式捕获清单防悬垂。
</details>

**3. static 局部变量会被 `[&]` 捕获吗？用什么方法可以实测验证？**
<details><summary>答案</summary>
不会。static 局部变量具有静态存储期，不在栈上，所有 lambda（包括 `[&]`）都直接引用静态区里那一份实体。验证方法：打印 lambda 内外的 `&s_count`（本文实测同为 `0x102ce0090`），再看 `sizeof(lambda)`——只有 8 字节，恰好是捕获的那个普通局部变量的引用大小，static 变量不占闭包空间。
</details>

**4. C++20 对 `[=]` 捕获 `this` 做了什么改动？**
<details><summary>答案</summary>
C++20 弃用了 `[=]` 对 `this` 的隐式捕获（`-Wdeprecated-this-capture`），要求显式写 `[=, this]` 表示「按指针捕获 this」，或写 `[*this]` 表示按值捕获整个对象。clang 23.1.0 实测：C++20 模式下裸 `[=]` 用成员变量给出弃用警告；`[=, this]` 在 C++17 模式下则是扩展警告。意图是让「捕获指针」这件事显式可见，堵住 `[=]` 的语义陷阱。
</details>

**5. 下面代码有什么问题？怎么修？**
```cpp
std::function<void()> make_task() {
    int x = 42;
    return [&] { std::cout << x; };
}
auto t = make_task();
t();
```
<details><summary>答案</summary>
悬垂引用，UB。`x` 是 `make_task` 的局部变量，函数返回即销毁；闭包用 `[&]` 捕获了 `x` 的引用，`t()` 调用时读的是已销毁的栈对象——本文同类代码实测打印垃圾值，编译器对直接返回场景会给 `-Wreturn-stack-address` 警告，但经 `std::function` 间接逃逸时警告可能缺席。修正：显式按值捕获 `return [x] { std::cout << x; };`。
</details>

**6. 想按值捕获成员的副本，C++11 / C++14 / C++17 各推荐什么写法？**
<details><summary>答案</summary>
C++11：`int local = data; return [local]{ ... };`——先拷到局部变量再按值捕获。C++14：init capture 一步到位 `return [d = data]{ ... };`（见 [Item 32](item32-init-capture.md)）。C++17：`return [*this]{ ... };` 按值捕获整个对象副本——对象大、只需个别成员时优先 C++14 写法，避免整对象拷贝。
</details>

---

## 参考与延伸

- 下一节：[Item 32 初始化捕获](item32-init-capture.md)
- 回到：[第 6 章 Lambda 表达式](README.md)
