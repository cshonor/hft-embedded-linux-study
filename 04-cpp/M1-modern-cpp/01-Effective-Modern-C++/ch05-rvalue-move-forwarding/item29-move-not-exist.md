# Item 29：认识移动操作不存在或廉价的情形

> 第 5 章 · Item 29 · 上一节：[Item 28 引用折叠](item28-reference-collapsing.md)

## 为什么要学这个（先建立直觉）

C 程序员知道 `memcpy` 对所有类型代价相同——都是按字节拷贝：

```c
struct Big { char data[4096]; };
struct Big a, b;
b = a;  // memcpy 4096 字节——没有"移动"的概念
```

C++ 有了移动语义，但**移动不是万能的**。有些类型没有移动操作，有些移动不比拷贝快。无脑 `std::move` 不总是有效：

```cpp
// std::array 的移动是 O(N)——因为它内联存储，没有指针可交接
std::array<int, 1000> arr;
auto arr2 = std::move(arr);  // 逐元素移动，O(1000)！

// vector 的移动是 O(1)——因为它内部是指针
std::vector<int> v;
auto v2 = std::move(v);  // 指针交接，O(1)
```

---

## 这节讲什么

移动不是万能的——有些类型没有移动操作，有些移动不比拷贝快。无脑 `std::move` 不总是有效。

---

## 三种"移动无效"的情形

### 1. 没有移动构造

```cpp
// 旧类、C 兼容结构体没有移动操作
struct LegacyData {
    int values[256];
    // 没有移动构造 → std::move 退回拷贝
};
LegacyData d;
auto d2 = std::move(d);  // 拷贝！因为没有移动构造

// C++ 类如果写了析构但没写移动，移动也被抑制（Item 17）
class OldWidget {
    int* data;
public:
    ~OldWidget() { delete[] data; }
    // 移动被抑制 → std::move 退回拷贝
};
```

### 2. 移动不比拷贝快

```cpp
// std::array 内联存储元素——移动是逐元素移动，O(N)
std::vector<std::array<int, 1000>> v;
auto v2 = std::move(v);  // vector 的移动是 O(1)
// 但 vector<array<int,1000>> 内部如果扩容，每个 array 的移动是 O(1000)

// 小类型（int、指针）移动 = 拷贝
int x = 42;
int y = std::move(x);  // 和 int y = x; 完全一样
```

### 3. 常量对象不能移动

```cpp
const std::string s = "hello";
auto s2 = std::move(s);  // 退回拷贝！const 对象不能移动
// 因为移动构造需要修改源对象（置空），const 不允许修改

const std::shared_ptr<Widget> sp;
auto sp2 = std::move(sp);  // 拷贝！const shared_ptr 不能移动
```

```cpp
std::vector<std::array<int, 1000>> v;
auto v2 = std::move(v);  // O(N)！array 的移动是逐元素的
```

---

## 常见错误（新手踩坑）

**错误 1：return std::move(local) 阻止 NRVO**
```cpp
Widget make() {
    Widget w;
    return std::move(w);  // 阻止 NRVO！强制走移动构造
}
// NRVO 比 move 更好——直接在返回值位置构造，零拷贝零移动
```
**修正：** `return w;`——让编译器做 NRVO。

**错误 2：对 array 无脑 move 以为 O(1)**
```cpp
std::array<BigStruct, 100> arr;
auto arr2 = std::move(arr);  // O(100 * sizeof(BigStruct))！不是 O(1)
// array 内联存储，没有指针可交接
```
**修正：** 用 `vector` 替代 `array` 如果需要移动，或接受 O(N)。

**错误 3：对 const 对象 move 期望移动**
```cpp
const std::vector<int> v = get_data();
auto v2 = std::move(v);  // 拷贝！const 不能移动
```
**修正：** 不要 `const` 如果想移动，或接受拷贝。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 资源转移 | 手动交换指针 | 移动构造（如果有） | C++ 语言级支持 |
| 拷贝代价 | 总是 O(n) | 移动 O(1) 或 O(n) | 取决于类型 |
| 返回值优化 | 不适用 | NRVO/RVO | 编译器直接构造 |
| const 限制 | 不适用 | const 不能移动 | 移动需要修改源 |

**一句话总结：** C 程序员记住——`std::move` 不是免费的。对 `array`、小类型、const 对象没用甚至有害。返回局部变量不要 `move`——让 NRVO 生效。

---

## HFT 关联

- **`vector<vector<Tick>>` 移动**：桶间用 `move` 转移是 O(1) 真收益；但桶内 `array<Tick, N>` 的移动是 O(N)——选 `vector` 而非 `array` 才能享受移动红利。
- **返回值优化**：HFT 函数返回大对象时不要 `return std::move(local)`——NRVO 比移动更好。
- **const 成员**：类的 `const` 成员会阻止移动构造（因为不能修改）——HFT 结构体避免用 `const` 成员。

---

## 自测题

1. `std::vector<std::array<int, 1000>>` 的移动是 O(1) 还是 O(N)？为什么？
2. 为什么 `return std::move(local)` 反而有害？
3. `const shared_ptr` 能移动吗？为什么？
4. 什么类型的移动和拷贝代价相同？
5. 下面代码有什么问题？
```cpp
std::array<BigStruct, 100> getData() {
    std::array<BigStruct, 100> arr;
    return std::move(arr);
}
```

<details>
<summary>参考答案</summary>

1. 移动整个 `vector` 是 **O(1)**，但移动其内部的每个 `array<int, 1000>` 元素是 **O(N)**（N = 1000）。原因：`std::vector` 只持有指向堆上数组的指针，移动就是把指针交接过来，与元素个数无关；而 `std::array` 把元素**内联存储**在对象内部，没有可交接的指针，它的移动构造只能逐个移动元素，对 `int` 元素来说就是逐元素拷贝。所以整体一次移动仍是 O(1) 的指针交接，可一旦需要搬移元素（如重分配）就是 O(N)——这也是笔记里"选 `vector` 而非 `array` 才能享受移动红利"的原因。

2. 有两个害处：①**阻止 NRVO/RVO**：`return` 一个与返回类型相同的局部变量时，编译器可以直接在调用方的返回值位置构造它（零拷贝、零移动）；而 `std::move(local)` 使返回表达式变成 `T&&`，不再是"具名局部对象"，NRVO 的前提被破坏，编译器只能走移动构造——白白多一次移动。②返回类型是引用/`T` 时还可能引入悬垂风险。正确写法就是 `return local;`。

3. 不能移动，会退化成**拷贝**。`std::move` 只是把实参转成 `T&&`，但 `const shared_ptr` 的类型是 `const std::shared_ptr<T>`，转成的是 `const std::shared_ptr<T>&&`。移动构造函数的形参是 `shared_ptr&&`（非 const），无法绑定 const 右值；于是重载决议选中拷贝构造 `shared_ptr(const shared_ptr&)`（它能绑定 const 实参）。根本原因是"移动需要修改源对象"，const 禁止修改。同理，类的 `const` 成员也会阻止编译器生成/使用移动构造。

4. 两类：①**没有间接资源的类型**：`std::array<T, N>`、`std::pair<int,int>`、所有元素都是标量的聚合体、POD 结构体——它们的数据就在对象里，移动只能逐成员复制，代价与拷贝相同（对 `int`/`double` 这类标量，移动就是拷贝）。②**移动被抑制/退化的情形**：`const` 对象、`const` 成员、没有定义移动操作的类型（退化成拷贝）、以及开启了 SSO 的小字符串（`std::string` 短字符串优化范围内，移动退化成一次小拷贝）。另外含原子/互斥成员的类型也不可移动。

5. 问题是画蛇添足且有害。`arr` 是 `std::array<BigStruct, 100>`：①它内联存储、没有指针可交接，移动是 O(100 × sizeof(BigStruct)) 的逐元素搬移，本来就省不了；②更关键的是 `return std::move(arr);` 破坏了 NRVO，编译器无法在返回值位置直接构造，只能先构造局部 `arr` 再移动出去，反而多了一次 O(N) 搬移。修正：直接 `return arr;`（让 NRVO 生效，理想情况下零拷贝），若确实要享受 O(1) 移动则改用 `std::vector<BigStruct>` 作为返回类型。

</details>

---

## 参考与延伸

- 下一节：[Item 30 完美转发失败](item30-forwarding-failures.md)
- 回到：[第 5 章](README.md)
