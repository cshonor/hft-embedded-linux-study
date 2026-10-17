# Item 2：不要试图写"容器无关"代码

> 第 1 章 容器 · Item 2 · 上一节：[Item 1 仔细选择容器](item01-choose-container.md) · 下一节：[Item 3 拷贝轻量且正确](item03-copy-lightweight-correct.md)

## 为什么要学这个（先建立直觉）

C 程序员习惯了"数据结构无关"的写法——用 `typedef` 一行就能切换数组类型：

```c
typedef int OrderId;
// 想换成 long？改 typedef 就行
OrderId ids[1000];
```

到了 C++，你可能会想：

```cpp
typedef std::vector<int> Container;
// 想换 list？改 typedef 就行？
Container c;
c.push_back(42);
```

看起来很美，但当你从 `vector` 换到 `map` 时——`push_back` 不存在了，`[]` 语义变了，迭代器失效规则完全不同。**"容器无关"是一个无法实现的理想。**

---

## 这节讲什么

序列容器（vector/deque/list）与关联容器（map/set）的接口差异巨大，迭代器失效规则也不同。试图用 `typedef` 抽象容器类型，会让你在切换时面临大量代码改写。务实做法：选一种容器，用 `typedef` 固定，性能不达标再换。

---

## 接口差异实例

```cpp
// 序列容器：push_back / pop_back / insert(pos, val)
std::vector<int> v;
v.push_back(42);           // ✅
v.insert(v.begin(), 0);    // ✅

// 关联容器：没有 push_back，用 insert(val) 或 emplace
std::set<int> s;
s.insert(42);              // ✅
// s.push_back(42);        // ❌ 编译错误

// vector 的 []：按下标随机访问
v[0] = 99;                 // ✅ O(1)

// map 的 []：按键查找/插入
std::map<int, std::string> m;
m[0] = "hello";            // ✅ 但语义是"按键插入"

// list 的迭代器：插入/删除不失效
auto it = l.begin();
l.insert(it, 42);          // it 仍然有效

// vector 的迭代器：插入可能全部失效
auto it = v.begin();
v.push_back(42);           // it 可能失效（扩容）！
```

---

## 常见错误（新手踩坑）

### 错误 1：用宏/typedef 试图同时兼容序列和关联容器

```cpp
// 试图写出"通用"代码
template<typename Container>
void add(Container& c, int val) {
    c.push_back(val);  // 对 set/map 编译失败
}
```

**修正：** 接受容器差异，针对每种容器写对应的操作。或用 `if constexpr` 分派。

### 错误 2：认为换容器只改 typedef

```cpp
typedef std::vector<Widget> Widgets;
Widgets w;
// ... 1000 行代码用了 push_back, [], at, iterator arithmetic ...

// 现在想换 list
typedef std::list<Widget> Widgets;  // 改了 typedef
// → 编译错误：[] 不存在、iterator arithmetic 不存在、reserve 不存在...
```

**修正：** 换容器前审计所有使用点。用 `typedef` 是好习惯（一处定义），但别以为它是"一键切换"。

### 错误 3：忽略迭代器失效规则的差异

```cpp
// vector 版本：收集要删的迭代器，批量 erase
std::vector<int>::iterator it;
// ... 收集逻辑 ...

// 换成 list 后，迭代器失效规则不同
// list erase 只失效被删元素，vector erase 失效之后所有
```

**修正：** 查阅目标容器的迭代器失效规则，修改删除逻辑。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 数据结构切换 | 改 typedef + 少量改写 | 接口/语义/失效规则全不同 | STL 容器不是统一接口 |
| 泛型 | 宏或 void* | 模板，但容器接口不统一 | 设计权衡 |
| 务实做法 | typedef 固定类型 | typedef 固定一种容器 | 换容器有成本 |

**一句话：** C 的数组操作是统一的（下标访问），但 STL 容器各有各的接口。`typedef` 能让代码更清晰，但不能让容器"可互换"。

---

## HFT 关联

- **容器选型一次定好**：HFT 系统在架构阶段就确定容器类型（通常 `vector` 为主），热路径代码深度依赖 `vector` 的连续性假设，换容器意味着重新优化。
- **typedef 固定类型**：用 `using OrderBook = std::vector<Level>;` 让代码自文档化，但团队都理解这是 `vector` 而非"可替换的容器"。

---

## 代码自测

### Q1: 接口差异
```cpp
std::vector<int> v = {1, 2, 3};
std::set<int> s = {1, 2, 3};

v.push_back(4);   // A
s.insert(4);       // B
// s.push_back(4); // C
```
> C 行会怎样？vector 和 set 在添加元素上有什么接口差异？

<details>
<summary>答案</summary>

C 行**编译错误**。`set` 没有 `push_back`——`set` 是有序的，插入位置由比较函数决定，不能指定。序列容器（vector/deque/list）有 `push_back`/`push_front`，关联容器只有 `insert`/`emplace`。
</details>

### Q2: 迭代器失效
```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin();
v.reserve(100);  // A

std::list<int> l = {1, 2, 3};
auto lit = l.begin();
l.push_back(4);  // B
```
> A 行后 it 有效吗？B 行后 lit 有效吗？

<details>
<summary>答案</summary>

- **A 后 it 可能失效**：`reserve` 如果触发扩容（新容量 > 旧容量），所有迭代器/指针/引用全部失效（内存搬迁）。
- **B 后 lit 仍然有效**：`list` 是节点容器，`push_back` 只新增节点，不影响已有节点的迭代器。
</details>

### Q3: typedef 的局限
```cpp
using Container = std::vector<int>;
Container c;
c.reserve(100);      // A
c[5] = 42;           // B
auto it = c.begin() + 3;  // C

// 如果改成 using Container = std::list<int>;
// 哪些行会编译失败？
```

<details>
<summary>答案</summary>

- **A 失败**：`list` 没有 `reserve`（链表不需要预分配）。
- **B 失败**：`list` 没有 `[]`（不是随机访问）。
- **C 失败**：`list::iterator` 是双向迭代器，不支持 `+ n`（只能 `++`/`--`）。

这说明换容器不是改一行 typedef 就行的。
</details>

### Q4: 实务建议
> 如果你的代码可能需要从 vector 切换到 list，最佳实践是什么？

<details>
<summary>答案</summary>

1. **先用 vector**：vector 在大多数场景下性能最好（cache 友好）。
2. **用 typedef 固定**：`using Container = std::vector<T>;`，代码用 typedef 名。
3. **封装操作**：把容器操作封装在函数里（`addItem(c, x)`），切换时只改函数实现。
4. **性能不达标再换**：profiler 证明 vector 是瓶颈才考虑换，否则过早优化。
</details>

---

## 参考与延伸

- 上一节：[Item 1 仔细选择容器](item01-choose-container.md)
- 下一节：[Item 3 拷贝轻量且正确](item03-copy-lightweight-correct.md)
- 回到：[第 1 章 容器](README.md)
