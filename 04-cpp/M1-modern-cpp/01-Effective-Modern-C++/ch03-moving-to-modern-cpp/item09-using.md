# Item 9：优先 using 别名而非 typedef

> 第 3 章 移步现代 C++ · Item 9 · 上一节：[Item 8 nullptr](item08-nullptr.md)

## 为什么要学这个（先建立直觉）

C 程序员对 `typedef` 很熟悉：

```c
typedef unsigned long ulong;
typedef struct { int x; int y; } Point;
typedef void (*Callback)(int, const char*);  // 函数指针别名
```

C++ 的 `typedef` 完全兼容 C，但 C++ 多了模板。当你想给"模板实例"起别名时，`typedef` 力不从心：

```cpp
// 想给 vector<int, MyAlloc<int>> 起个短名
typedef std::vector<int, MyAlloc<int>> IntVec;  // OK，但只对 int 固定了

// 想做成模板别名？typedef 做不到！
template<class T> typedef std::vector<T, MyAlloc<T>> Vec;  // 编译失败！
```

C++11 的 `using` 语法解决了这个问题，而且语法更直观（从左到右读，像 `auto`）：

```cpp
template<class T> using Vec = std::vector<T, MyAlloc<T>>;  // OK！
Vec<int> v;  // 等价于 vector<int, MyAlloc<int>>
```

---

## 这节讲什么

`using` 和 `typedef` 功能相同，但 `using` 支持模板化（alias template），`typedef` 不行。且 `using` 的语法更直观。

---

## 核心区别

### 普通别名：两者等价

```cpp
typedef unsigned long ulong;     // typedef：原类型在后面
using ulong = unsigned long;     // using：新名在左边，原类型在右边

// 函数指针：using 明显更清晰
typedef void (*Callback)(int, std::string);    // 难读——名字藏在中间
using Callback = void(*)(int, std::string);     // 清晰——名字在左边
```

### 模板别名：只有 using 可以

```cpp
// typedef 做不到——C++ 没有模板 typedef 语法
// template<class T> typedef std::vector<T, MyAlloc<T>> Vec;  // 编译失败！

// using 直接搞定
template<class T> using Vec = std::vector<T, MyAlloc<T>>;
Vec<int> v1;           // vector<int, MyAlloc<int>>
Vec<std::string> v2;   // vector<string, MyAlloc<string>>

// 更复杂的例子：函数返回类型别名
template<class T> using Owner = std::unique_ptr<T>;
Owner<Widget> w = std::make_unique<Widget>();  // unique_ptr<Widget>
```

### 在模板元编程中的优势

```cpp
// 用 typedef 在模板里取别名需要 typename + 嵌套
template<class Container>
void process(Container& c) {
    typedef typename Container::value_type VT;  // 需要 typename
    VT x = c[0];
}

// 用 using 更简洁
template<class Container>
void process(Container& c) {
    using VT = typename Container::value_type;  // 同样需要 typename，但语法更清晰
    VT x = c[0];
}

// 真正的优势：alias template 可以做模板元编程
template<class T> using RemoveRef = typename std::remove_reference<T>::type;
RemoveRef<int&> x;  // x 是 int
```

---

## 常见错误（新手踩坑）

**错误 1：在模板里用 typedef 忘加 typename**
```cpp
template<class Container>
void f(Container& c) {
    typedef Container::value_type VT;  // 编译失败！依赖类型需要 typename
    typedef typename Container::value_type VT;  // OK
}
```
**修正：** 依赖类型必须加 `typename`。`using` 也一样要加。

**错误 2：以为 typedef 能做模板别名**
```cpp
template<class T> typedef std::shared_ptr<T> SP;  // 编译失败
```
**修正：** 用 `template<class T> using SP = std::shared_ptr<T>;`。

**错误 3：函数指针 typedef 读不懂**
```cpp
typedef bool (*Compare)(const std::string&, const std::string&);  // 名字在哪？
```
**修正：** 用 `using Compare = bool(*)(const std::string&, const std::string&);`，名字在左边一目了然。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 类型别名 | `typedef` | `using` | `using` 语法更直观，支持模板 |
| 模板别名 | 不适用（C 无模板） | `template<class T> using X = ...` | `typedef` 做不到 |
| 函数指针别名 | `typedef void (*F)(int);` | `using F = void(*)(int);` | `using` 名字在左边，更易读 |
| 依赖类型 | 不适用 | 需要 `typename` 前缀 | 模板里嵌套类型是依赖类型 |

**一句话总结：** C 程序员把 `typedef` 换成 `using` 就行，语法是 `using 新名 = 原类型`，从左到右读，和 `auto` 一样自然。

---

## HFT 关联

- **自定义分配器容器**：`template<class T> using PoolVec = std::vector<T, PoolAlloc<T>>;` 让 HFT 内存池容器有简洁的别名。
- **回调类型**：`using TickCallback = void(*)(const Tick&);` 比 `typedef void (*TickCallback)(const Tick&);` 清晰得多。
- **智能指针别名**：`template<class T> using Ref = std::shared_ptr<T>;` 在策略代码中简化书写。

---

## 自测题

1. `using` 和 `typedef` 在普通别名上有什么区别？在模板别名上呢？
2. 为什么 `typedef` 不能定义模板别名而 `using` 可以？
3. `using Callback = void(*)(int);` 比 `typedef void (*Callback)(int);` 好在哪？
4. 下面代码有什么问题？
```cpp
template<class T>
void f(T container) {
    typedef T::value_type VT;  // 这里会编译失败吗？
    VT x;
}
```
5. 用 `using` 写一个模板别名，让 `HashMap<K,V>` 等价于 `std::unordered_map<K, V, CustomHash<K>>`。

<details>
<summary>参考答案</summary>

1. 普通别名上二者**语义完全等价**，`using X = Y;` 与 `typedef Y X;` 声明的是同一个类型，没有性能或行为差别；区别只在可读性和书写习惯。模板别名上则有本质差别：`using` 支持带模板参数（`template<class T> using X = ...`），可以直接实例化出别名模板；`typedef` 没有别名模板语法，做不到。

2. 因为 `typedef` 的语法本质是"为一个**已确定的类型**起名字"（`typedef 类型 名字;`），它没有位置放置模板参数列表，也无法生成"带参数的别名"。`using` 的别名语法是"名字 = 类型"，天然可以在前面加 `template<...>` 形成别名模板（alias template）。要在 C++11 之前用 typedef 模拟，只能包一层 `struct` + 内部 `typedef`，使用时还要写 `typename`，非常笨拙。

3. 可读性：`using` 的形式是"新名字在左边、类型在右边"，和 `T x = expr;` 的书写方向一致，函数指针这种"名字被类型包在中间"的写法尤其明显——`using Callback = void(*)(int);` 一眼看出 `Callback` 是"接受 int、返回 void 的函数指针"；`typedef void (*Callback)(int);` 名字藏在中间，读起来要拆。此外 `using` 能直接模板化，`typedef` 不能。

4. 会编译失败。`T::value_type` 是**依赖类型**（dependent type），编译器在解析模板定义时不知道 `T` 是什么，无法判断 `T::value_type` 是类型还是静态成员，必须显式加 `typename` 前缀。正确写法：
```cpp
template<class T>
void f(T container) {
    typename T::value_type x;          // 或
    using VT = typename T::value_type; // using 同样需要 typename
    VT y;
}
```
注意：即使换用 `using`，依赖类型的 `typename` 也不能省（C++20 起在部分语境可省略）。

5. 用别名模板：
```cpp
template<class K, class V>
using HashMap = std::unordered_map<K, V, CustomHash<K>>;

HashMap<std::string, Order> orders;   // 即 unordered_map<string, Order, CustomHash<string>>
```
`using` 声明的别名模板可以直接出现在模板实参推导和特化中，这是 `typedef` 无法做到的。

</details>

---

## 参考与延伸

- 下一节：[Item 10 scoped enum](item10-scoped-enum.md)
- 回到：[第 3 章 移步现代 C++](README.md)
