# Item 11：优先使用 =delete 而非 private 未定义

> 第 3 章 移步现代 C++ · Item 11 · 上一节：[Item 10 scoped enum](item10-scoped-enum.md)

## 为什么要学这个（先建立直觉）

C++ 经常需要表达"这个操作不许用"：不可拷贝的锁、不可赋值的连接句柄、不接受 `double` 的整数函数。C++98 时代的惯用法是**把函数声明为 private 且不定义**——编译器看到 private 会拦外部调用，没定义所以链接器拦内部调用。这套"组合拳"能用，但处处别扭：

第一，错误暴露得**晚**。外部调用确实编译期就报"private 访问"，但成员函数和友元里的调用能过编译，错误要等到**链接期**才炸——`Undefined symbols` 这种报错离真正的错误现场（哪行代码试图拷贝）隔了十万八千里。第二，手法**只适用于成员函数**。想禁掉一个普通函数的错误重载（比如 `isLucky(double)`），private 根本不适用。第三，意图**藏得深**：读代码的人要同时注意到"private + 没有定义"两个事实才能看懂设计意图。

C++11 的 `= delete` 把"禁用"变成一等语法：任何函数（成员、非成员、模板实例化）都能在声明处直接标记删除，**所有**调用——不管来自哪里——都在编译期得到一条清晰的 `call to deleted function` 报错。

---

## 这节讲什么

1. C++98 `private` 未定义手法的链接期报错 vs `= delete` 的编译期报错（附两边实测原文）；
2. `= delete` 删非成员函数：`isLucky` 挡 `bool`/`char`/浮点的实战；
3. `= delete` 删模板实例化（private 手法做不到的）；
4. 规则细节：`= delete` 必须出现在首次声明处；
5. 全部论断本机实测：clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`。

---

## 核心对决：链接期报错 vs 编译期报错

### C++98 手法：错误在链接期才爆

```cpp
class Widget {
public:
    Widget() {}
    void touch(const Widget& other) {
        Widget copy(other);   // 成员函数有权访问 private → 编译期放行
        (void)copy;
    }
private:
    Widget(const Widget&);    // 只声明，不定义
};

int main() {
    Widget a;
    a.touch(a);
}
```

本机实测 clang 23.1.0：编译全部通过，**链接期**才报错：

```
Undefined symbols for architecture arm64:
  "Widget::Widget(Widget const&)", referenced from:
      Widget::touch(Widget const&) in emcA_item11_cpp98-23ed8d.o
ld: symbol(s) not found for architecture arm64
clang++: error: linker command failed with exit code 1
```

报错只告诉你"缺 `Widget::Widget(Widget const&)` 的定义"，至于这条调用从哪来、是故意禁用还是忘了实现，全靠自己查。而且这条路径绕过了编译期检查——意味着"禁拷贝"的纪律对成员和友元是漏的。

### = delete：编译期、指名道姓

```cpp
class Widget {
public:
    Widget() {}
    Widget(const Widget&) = delete;   // 声明处直接禁用
    void touch(const Widget& other) {
        Widget copy(other);
        (void)copy;
    }
};
```

本机实测 clang 23.1.0 报错原文：

```
error: call to deleted constructor of 'Widget'
    Widget copy(other);
           ^    ~~~~~
note: 'Widget' has been explicitly marked deleted here
    Widget(const Widget&) = delete;
    ^
```

对比一目了然：**编译期**而非链接期；报错直接指出"调用了被显式删除的函数"并给出删除位置；成员函数、友元、外部调用一视同仁，没有漏网之鱼。

| 维度 | private 未定义（C++98） | `= delete`（C++11） |
|------|------------------------|---------------------|
| 报错时机 | 外部调用编译期；成员/友元调用**链接期** | 一律编译期 |
| 报错信息 | `Undefined symbols`，不指向意图 | `call to deleted function`，指向删除处 |
| 适用范围 | 仅成员函数 | 任何函数：成员、非成员、模板实例化 |
| 意图表达 | 靠"private + 不定义"两个暗示 | 声明处一行，一目了然 |

---

## 删非成员函数：isLucky 实战

`private` 手法管不了自由函数，但自由函数恰恰最需要"禁用某些重载"——隐式转换会把你没想服务的类型吸进来：

```cpp
bool isLucky(int number) { return number == 7; }
bool isLucky(char) = delete;      // 挡 char（否则 'a' 隐式转 int）
bool isLucky(bool) = delete;      // 挡 bool（否则 true 隐式转 1）
bool isLucky(double) = delete;    // 挡 double/float（否则 3.5 截断成 3）
```

本机实测 clang 23.1.0，`isLucky('a')` 报错：

```
error: call to deleted function 'isLucky'
    return isLucky('a');
           ^~~~~~~
note: candidate function has been explicitly deleted
    bool isLucky(char) = delete;
         ^
```

`isLucky(3.5f)` 同样被拦——`float` 没有精确匹配，`float→double`（浮点转换）优先级高于 `float→int`（浮点-整型转换），重载决议选中 `isLucky(double)`，正好撞上 `= delete`：

```
error: call to deleted function 'isLucky'
    return isLucky(3.5f);
           ^~~~~~~
note: candidate function has been explicitly deleted
    bool isLucky(double) = delete;
         ^
```

删掉一个 `double` 重载，顺手把 `float` 也挡了——这是利用重载决议规则"借力打力"：被删除的函数**参与**重载决议（可能比别人更匹配），只是选中即报错。

---

## 删模板实例化：private 手法做不到的

`void*` 能装任何对象指针，但有些接口不接受它（`void*` 丢掉了类型信息，函数体没法处理）。`private` 对模板特化无能为力，`= delete` 可以：

```cpp
template<typename T>
void processPointer(T* ptr) { /* ... */ }

template<>
void processPointer<void>(void*) = delete;

template<>
void processPointer<const void>(const void*) = delete;
```

实测：`processPointer(&x)`（`int*`）正常走主模板输出 `processPointer<T>`；`processPointer(vp)`（`void*`）编译失败，报 `call to deleted function`。同理还能删 `char*`/`const char*` 实例化，防止"字符串指针混进只接受对象指针的泛型接口"。

---

## 规则细节：= delete 必须在首次声明处

`= delete` 不能像普通定义那样"类内声明、类外补"：

```cpp
class Widget {
public:
    Widget();
    Widget(const Widget&);          // 首次声明没带 = delete
};
Widget::Widget(const Widget&) = delete;   // 类外补：报错
```

本机实测 clang 23.1.0 报错原文：

```
error: deleted definition must be first declaration
    Widget::Widget(const Widget&) = delete;
                                    ^
note: previous declaration is here
    Widget(const Widget&);
    ^
```

理由很实在：编译器必须在**第一次看到**函数声明时就知道它被删了，这样后面的每次调用才能立刻诊断；如果允许事后补删，调用点的合法性会随声明顺序变化，两阶段查找会变成噩梦。

顺带一句 `= default`：`C() = default;` 让编译器生成默认实现且不算"用户提供"，从而保持 trivial 性质（可 `memcpy`、可 `constexpr`），而手写 `C() {}` 会破坏它——详见 [Item 17 特殊成员函数的生成规则](item17-special-member-functions.md)。

---

## 完整可运行例子

```cpp
#include <iostream>

class Connection {                       // 不可拷贝的连接句柄
public:
    explicit Connection(int fd) : fd_(fd) {}
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = default;  // 但允许移动
    int fd() const { return fd_; }
private:
    int fd_;
};

bool isLucky(int n) { return n == 7; }
bool isLucky(char) = delete;
bool isLucky(bool) = delete;
bool isLucky(double) = delete;

template<typename T>
void processPointer(T*) { std::cout << "processPointer<T>\n"; }
template<> void processPointer<void>(void*) = delete;

int main() {
    Connection c1(3);
    Connection c2(std::move(c1));        // 移动 OK
    std::cout << "c2.fd=" << c2.fd() << "\n";
    // Connection c3(c2);                // error: call to deleted constructor

    std::cout << "isLucky(7)=" << isLucky(7) << "\n";
    // isLucky('a');   isLucky(true);   isLucky(3.5);   // 全被 delete 拦下

    int x = 0;
    processPointer(&x);                  // OK
    // processPointer((void*)&x);        // error: deleted specialization
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
c2.fd=3
isLucky(7)=1
processPointer<T>
```

---

## 常见错误（新手踩坑）

1. **类外补 `= delete`**。实测报 `error: deleted definition must be first declaration`——删除标记必须写在首次声明处，修正：类内声明时直接带上。
2. **以为被删函数不参与重载决议**。恰恰相反：被删函数照常参与，而且可能因为更匹配而"胜出"，然后报 `call to deleted function`。这正是 `isLucky(double) = delete` 能顺手挡住 `float` 的原理，也是新手看到报错时困惑的原因。
3. **用 private 未定义手法禁模板实例化**。办不到——`template<> void f<void>(void*);` 声明为 private 不影响外部对主模板的实例化路径。修正：`template<> void f<void>(void*) = delete;`。
4. **删拷贝构造但忘了拷贝赋值**。`T(const T&) = delete;` 不会连带删 `operator=`。要禁拷贝就两个都删（或删一个声明一个，见 Item 17 的生成规则联动）。

---

## HFT 关联

- **句柄类禁拷贝**：连接、内存池句柄、独占锁都该 `= delete` 拷贝构造/赋值——双重 `close(fd)`、双重 `munmap` 这类事故在编译期清零。保留 `= default` 移动语义，所有权转移零成本。
- **接口收窄防隐式转换**：行情解码函数 `decode(const ItchMsg&)` 旁边删掉 `decode(const char*) = delete;`，防止原始 buffer 指针被误传后按错误协议解析——低延迟系统里"解析错了但没崩"比崩溃可怕得多。
- **热路径零开销**：`= delete` 是纯编译期机制，生成的二进制里没有任何痕迹；相比运行时断言（`assert(ptr)`），它连一个分支都不占。
- **模板实例化白名单**：泛型撮合组件用 `= delete` 删掉不支持的类型实例化（如 `void*`、`const char*`），把"这个模板不是给所有类型用的"从文档注释升级成编译器强制。

---

## 自测题

**1. C++98 禁拷贝的标准手法是什么？它最大的缺陷是什么？**
<details><summary>答案</summary>
把拷贝构造/赋值声明为 private 且不定义。缺陷：成员函数和友元里的拷贝能通过编译，错误要到链接期才暴露（实测 `Undefined symbols: "Widget::Widget(Widget const&)"`），报错不指向错误意图，排查成本高。
</details>

**2. `= delete` 相比 private 未定义，报错上有什么优势？**
<details><summary>答案</summary>
一律编译期报错，且信息明确指向意图：实测 `error: call to deleted constructor of 'Widget'` + `note: 'Widget' has been explicitly marked deleted here`。private 手法在成员/友元调用路径上是链接期的 `Undefined symbols`，不含"被禁用"的语义。
</details>

**3. `isLucky(int)` 已存在，为什么要删掉 `isLucky(double)` 而不是 `isLucky(float)`？**
<details><summary>答案</summary>
因为重载决议中 `float→double` 是浮点转换，优先级高于 `float→int` 的浮点-整型转换——`isLucky(3.5f)` 会选中 `isLucky(double)`。删掉 `double` 版本后，`float` 实参撞到被删函数上直接报错（实测 `call to deleted function`），等于一个 `= delete` 挡住整个浮点家族。
</details>

**4. 被 `= delete` 的函数还参与重载决议吗？**
<details><summary>答案</summary>
参与。被删函数和其他候选一样进入候选集，甚至可能因更匹配而胜出——胜出后才报 `call to deleted function`。这是`= delete` 能用来"收窄重载集"的原理。
</details>

**5. 怎么禁止某个模板被 `void*` 实例化？private 能做到吗？**
<details><summary>答案</summary>
写显式特化并删除：`template<> void processPointer<void>(void*) = delete;`。private 做不到——它只能限制成员的访问权限，管不住模板的实例化路径。
</details>

**6. `Widget::Widget(const Widget&) = delete;` 写在类外会怎样？**
<details><summary>答案</summary>
编译失败：实测 `error: deleted definition must be first declaration`。`= delete`（以及类内 `= default`）必须出现在函数的首次声明处，编译器要求第一次看到声明就知道函数被删，否则调用点合法性会依赖声明顺序。
</details>

**7. `C() {}` 和 `C() = default;` 对 trivial 性质的影响有何不同？**
<details><summary>答案</summary>
`C() {}` 是用户提供的构造函数，使类型失去 trivial 性质（影响 `memcpy` 合法性与 ABI 传递）；`C() = default;` 由编译器生成、不算用户提供，成员都 trivial 时类型保持 trivial。详见 Item 17。
</details>

---

## 参考与延伸

- 上一节：[Item 10 scoped enum](item10-scoped-enum.md)
- 下一节：[Item 12 override](item12-override.md)
- 相关：[Item 17 特殊成员函数的生成规则](item17-special-member-functions.md)（`= delete`/`= default` 与编译器自动生成的联动）
- 回到：[第 3 章 移步现代 C++](README.md)
