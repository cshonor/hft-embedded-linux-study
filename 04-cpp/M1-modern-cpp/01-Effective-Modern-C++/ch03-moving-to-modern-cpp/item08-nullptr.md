# Item 8：优先 nullptr 而非 0 和 NULL

> 第 3 章 移步现代 C++ · Item 8 · 上一节：[Item 7 () vs {}](item07-parens-vs-braces.md)

## 为什么要学这个（先建立直觉）

C 里写空指针就两招：`int* p = NULL;` 或 `int* p = 0;`。在 C 里这基本没问题——C 没有函数重载，传 `NULL` 给函数就是传空指针，世界很单纯。

C++ 有重载，事情就变了味。`f(0)` 里的 `0` 是个如假包换的 `int`——如果你的重载集里恰好有 `f(int)` 和 `f(void*)`，编译器会毫不犹豫地选 `f(int)`，哪怕你的意图是"传个空指针"。`NULL` 也好不到哪去：C++ 不允许 `void*` 隐式转其他指针，所以 C++ 标准库只能把 `NULL` 定义成整型字面量（本机 macOS 上是 `0L`，即 `long`）。它**从来不是指针类型**。

最扎心的是模板：把 `NULL` 传进 `template<class T> void g(T)`，`T` 被推导成 `long`——函数体里拿到的是个整数，想赋给指针直接编译失败。这种 bug 在泛型代码里藏得很深。

`nullptr` 是 C++11 给出的答案：一个有自己的类型 `std::nullptr_t` 的空指针字面量，能隐式转成任何指针，但**永远不能**变成整数。歧义从根上消失。

---

## 这节讲什么

1. `0` / `NULL` / `nullptr` 在重载决议下的真实走向（附实测）；
2. `NULL` 在模板里被推导成整数的坑（呼应 Item 30 的完美转发失败）；
3. `std::nullptr_t` 的性质：`sizeof`、隐式转换规则；
4. 全部论断本机实测：clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`。

---

## 重载决议实测：三个实参三种命运

```cpp
#include <iostream>

void f(int)   { std::cout << "f(int)\n"; }
void f(bool)  { std::cout << "f(bool)\n"; }
void f(void*) { std::cout << "f(void*)\n"; }

int main() {
    f(0);         // 哪个？
    // f(NULL);   // 这行根本编不过（见下）
    f(nullptr);   // 哪个？
}
```

本机实测 clang 23.1.0 输出：

```
f(int)
f(void*)
```

- `f(0)` → `f(int)`：`0` 是 `int` 字面量，精确匹配整型重载，指针重载看都不看。
- `f(nullptr)` → `f(void*)`：`nullptr` 只能转指针，整型重载出局。

而 `f(NULL)` 在这组重载下**直接编译失败**：

```
error: call to 'f' is ambiguous
    f(NULL);
    ^
note: candidate function     void f(int)
note: candidate function     void f(bool)
note: candidate function     void f(void*)
```

本机 `NULL` 展开为 `0L`（`long`）：`long→int`、`long→bool`、`long→void*`（`0L` 是空指针常量）三条转换路径**同分**，重载决议判平局。这就是 `NULL` 的尴尬处境——它既不够像整数，也不够像指针，在哪边都是半个候选人。如果重载集里碰巧没有 `f(bool)`，`f(NULL)` 会静默选中 `f(int)`——bug 就从编译期错误变成运行期错误。

---

## 模板推导：NULL 变成整数的地方

```cpp
#include <iostream>

template<typename T>
void g(T x) { std::cout << __PRETTY_FUNCTION__ << "\n"; }

int main() {
    g(0);         // ?
    g(NULL);      // ?
    g(nullptr);   // ?
}
```

本机实测 clang 23.1.0 输出：

```
void g(T) [T = int]
void g(T) [T = long]
void g(T) [T = std::nullptr_t]
```

模板推导只看表达式的类型：`0` 是 `int`，`NULL`（`0L`）是 `long`，都和指针无关。函数体里如果写 `*x` 或 `x->foo()`，编译报错的位置在模板深处，新手很难联想到是调用点传了 `NULL`。只有 `g(nullptr)` 推出 `T = std::nullptr_t`，函数体里可以再隐式转成需要的任意指针类型。

这正是完美转发里"`NULL`/`0` 无法正确转发空指针"的根源，详见 [Item 30 完美转发失败的情形](../ch05-rvalue-move-forwarding/item30-forwarding-failures.md)。泛型代码里表达"空指针"只有一个选择：`nullptr`。

---

## std::nullptr_t：nullptr 的类型

```cpp
#include <cstddef>
#include <iostream>

int main() {
    std::cout << "sizeof(nullptr)=" << sizeof(nullptr) << "\n";
    std::cout << "sizeof(void*)=" << sizeof(void*) << "\n";
    std::cout << "sizeof(NULL)=" << sizeof(NULL) << "\n";

    std::nullptr_t np = nullptr;   // 可以声明该类型的变量
    int* p = np;                   // nullptr_t 可隐式转任意指针
    std::cout << "p==nullptr: " << (p == nullptr) << "\n";
}
```

本机实测 clang 23.1.0 输出：

```
sizeof(nullptr)=8
sizeof(void*)=8
sizeof(NULL)=8
```

要点：

- `std::nullptr_t` 大小和指针相同（本机 8 字节），传参、存容器没有额外开销；
- 它能隐式转**任意**对象指针、函数指针和成员指针，也能和任意指针比较（`==`/`!=`）；
- 转整型是**禁止**的：`int x = nullptr;` 实测报错 `error: cannot initialize a variable of type 'int' with an rvalue of type 'std::nullptr_t'`；
- 语境转换到 `bool` 是允许的：`if (p)`、`if (!p)` 照常工作，`nullptr` 判为 `false`。

对比表：

| 写法 | 类型（本机） | 重载中像整数？ | 模板推导 | 可读性 |
|------|------------|--------------|---------|--------|
| `0` | `int` | 是（就是它） | `T = int` | 差，要看上下文 |
| `NULL` | `long`（`0L`） | 是，还可能 ambiguous | `T = long` | 中，但语义是假的 |
| `nullptr` | `std::nullptr_t` | 否 | `T = std::nullptr_t` | 一眼即空指针 |

---

## 完整可运行例子

```cpp
#include <cstddef>
#include <iostream>
#include <type_traits>

void log(int id)     { std::cout << "log(int): id=" << id << "\n"; }
void log(void* ptr)  { std::cout << "log(void*): ptr=" << ptr << "\n"; }

template<typename T>
void deduce(T x) { std::cout << __PRETTY_FUNCTION__ << "\n"; }

int main() {
    // 1. 重载走向
    log(0);         // log(int)：0 是整数
    log(nullptr);   // log(void*)：nullptr 只转指针

    // 2. 模板推导
    deduce(0);
    deduce(NULL);
    deduce(nullptr);

    // 3. nullptr_t 的性质
    static_assert(std::is_same_v<decltype(nullptr), std::nullptr_t>);
    static_assert(sizeof(nullptr) == sizeof(void*));
    std::nullptr_t np = nullptr;
    int* p = np;
    std::cout << "p is null: " << std::boolalpha << (p == nullptr) << "\n";

    // 4. 禁止转整型（编译期错误，本例注释掉才能跑）
    // int bad = nullptr;   // error: cannot initialize 'int' with 'std::nullptr_t'
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
log(int): id=0
log(void*): ptr=0
void deduce(T) [T = int]
void deduce(T) [T = long]
void deduce(T) [T = std::nullptr_t]
p is null: true
```

---

## 常见错误（新手踩坑）

1. **重载集里传 `0` 想表达空指针**。`process(0)` 选中的永远是 `process(int)` 而不是 `process(Widget*)`。修正：`process(nullptr)`。
2. **模板里传 `NULL`**。实测 `T` 被推导成 `long`，函数体把它当指针用时报错埋在模板实例化深处。修正：模板场景一律 `nullptr`。
3. **以为 `NULL` 是指针类型**。C 里 `NULL` 常是 `((void*)0)`，但 C++ 禁止 `void*` 隐式转对象指针，所以 C++ 的 `NULL` 只能是整型（本机 `0L`）。从 C 带来的直觉在重载和模板里都会翻车。
4. **同一文件混用三种写法**。`if (p == NULL)` 和 `if (p == nullptr)` 并存，审查者要分心判断每一处有没有重载/推导陷阱。修正：全仓统一 `nullptr`，老代码顺手改，`NULL` 在 C++ 里没有任何 `nullptr` 不具备的优势。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 空指针写法 | `NULL` 或 `0` | `nullptr` | C++ 有重载与模板，`0`/`NULL` 是整型会误选 |
| `NULL` 的定义 | `((void*)0)`（指针类型） | `0` 或 `0L`（整型） | C++ 不允许 `void*` 隐式转其他指针 |
| 模板推导 | 不适用（C 无模板） | `nullptr` → `T = std::nullptr_t` | 正确表达"空指针"意图 |

**一句话总结：** C 程序员记住——C++ 的 `NULL` 不是 C 的 `NULL`（C 的是指针、C++ 的是整型），C++ 里空指针一律写 `nullptr`。

---

## HFT 关联

- **回调注册/清除**：`set_handler(nullptr)` 清除行情回调，重载决议与模板推导都零歧义；`set_handler(0)` 在模板化回调框架里会把 `T` 推成 `int`，编译失败都算好运，推到别的重载才是事故。
- **哨兵节点清晰化**：侵入式链表/内存池 freelist 的尾哨兵写 `nullptr`，读汇编时就是一次与 0 的比较（`cbz`/`cbnz`），和 `NULL` 生成的代码完全一样——零成本，纯赚可读性。汇编级分析见姊妹篇 [Item 8a nullptr 的硬件路径](item08a-nullptr的硬件路径.md)。
- **ABI 无差异**：`sizeof(nullptr) == sizeof(void*)`（实测 8），用 `nullptr` 替换 `NULL` 不改变任何二进制布局，存量代码可以放心机械化替换。

---

## 自测题

**1. `void f(int); void f(void*);` 下 `f(0)`、`f(NULL)`、`f(nullptr)` 各调哪个？**
<details><summary>答案</summary>
`f(0)` 调 `f(int)`（`0` 是 `int` 字面量，精确匹配）；`f(nullptr)` 调 `f(void*)`（`nullptr` 只能转指针）；`f(NULL)` 视实现而定——本机 `NULL` 是 `0L`，实测在两参重载里尚可调 `f(int)`，加上 `f(bool)` 后变成 `error: call to 'f' is ambiguous`。结论：只有 `nullptr` 的行为不依赖实现。
</details>

**2. `nullptr` 的类型是什么？能隐式转成 `int` 吗？**
<details><summary>答案</summary>
类型是 `std::nullptr_t`（`<cstddef>`）。不能转 `int`：实测 `int x = nullptr;` 报 `error: cannot initialize a variable of type 'int' with an rvalue of type 'std::nullptr_t'`。它能隐式转任意指针类型，语境下可转 `bool`（`if (p)`）。
</details>

**3. `template<class T> void g(T); g(NULL);` 在本机推出 `T` 是什么？后果是什么？**
<details><summary>答案</summary>
`T = long`（本机 `NULL` 是 `0L`）。后果：函数体里拿到的"指针"其实是个整数，任何指针运算（`*x`、`x->`、赋给指针变量）都会编译失败，且报错位置在模板深处。这就是 Item 30 讲的完美转发失败场景之一。
</details>

**4. 为什么 C++ 不把 `NULL` 定义成 `((void*)0)`，像 C 那样？**
<details><summary>答案</summary>
因为 C++ 禁止 `void*` 隐式转换为其他对象指针类型。若 `NULL` 是 `((void*)0)`，`int* p = NULL;` 都编不过。所以 C++ 标准规定 `NULL` 是整型空指针常量（`0` 或 `0L`），它进入重载决议时天然是"整数候选人"，这是一切歧义的根源。
</details>

**5. `sizeof(nullptr)` 是多少？把代码里的 `NULL` 全换成 `nullptr` 会改变 ABI 吗？**
<details><summary>答案</summary>
本机实测 8，与 `sizeof(void*)` 相同。不会改变 ABI：`nullptr` 的表示就是空指针值，和 `NULL` 生成的机器码一致（见 Item 8a 的汇编对比），替换是纯语义增强。
</details>

**6. 下面代码有什么问题？**
```cpp
void register_cb(int id);
void register_cb(void(*cb)());
register_cb(0);
```
<details><summary>答案</summary>
`register_cb(0)` 选中的是 `register_cb(int)`——程序员意图多半是"清除回调"，实际调用的是"注册 id 0"。修正：`register_cb(nullptr)` 明确选中函数指针重载，编译期保证意图与行为一致。
</details>

---

## 参考与延伸

- 上一节：[Item 7 () vs {}](item07-parens-vs-braces.md)
- 配套深挖：[Item 8a nullptr 的硬件路径——判空、重载与 0 页保护（汇编实测）](item08a-nullptr的硬件路径.md)
- 下一节：[Item 9 using 别名](item09-using.md)
- 相关：[Item 30 完美转发失败的情形](../ch05-rvalue-move-forwarding/item30-forwarding-failures.md)
- 回到：[第 3 章 移步现代 C++](README.md)
