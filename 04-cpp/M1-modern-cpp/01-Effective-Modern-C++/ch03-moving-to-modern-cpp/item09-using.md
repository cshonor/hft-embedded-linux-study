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

---

## 参考与延伸

- 下一节：[Item 10 scoped enum](item10-scoped-enum.md)
- 回到：[第 3 章 移步现代 C++](README.md)
