# Item 3：使容器里对象的拷贝轻量且正确

> 第 1 章 容器 · Item 3 · 上一节：[Item 2 不要写容器无关代码](item02-no-container-agnostic.md) · 下一节：[Item 4 empty() 而非 size()==0](item04-empty-not-size-zero.md)

## 为什么要学这个（先建立直觉）

C 程序员把数据放进数组时，就是 `memcpy` 一下：

```c
struct Order orders[100];
orders[0] = some_order;  // 结构体赋值 = memcpy
```

C++ 容器插入元素时也是**按值拷贝**，但"拷贝"可能调用**拷贝构造函数**——它可能分配内存、加锁、深拷贝。如果对象拷贝代价高，容器操作的底座就塌了。

```cpp
std::vector<BigObject> v;
v.push_back(obj);  // 调用 BigObject 的拷贝构造函数 → 可能 malloc + memcpy
```

更阴险的是**对象切片**——把派生类存进基类容器，派生部分直接丢失。

---

## 这节讲什么

容器插入是按值拷贝（调用拷贝构造函数）。元素拷贝代价 = 容器操作代价的底座。对策：存指针/智能指针（拷贝廉价）、用 `emplace` 直接构造、用移动语义。多态对象必须存指针，否则对象切片。

---

## 拷贝的代价

```cpp
struct BigObject {
    int data[1000];
};

std::vector<BigObject> v;
for (int i = 0; i < 100; ++i)
    v.push_back(obj);  // 每次 push_back 都拷贝 4000 字节
// 如果扩容 10 次 → 100 × 4000 + 10 × 100 × 4000 = 440 万字节拷贝
```

### 对象切片

```cpp
class Base { public: int b; virtual void print() { puts("Base"); } };
class Derived : public Base { public: int d; void print() override { puts("Derived"); } };

std::vector<Base> v;
v.push_back(Derived{1, 2});  // 拷贝构造 Base(const Base&)
// Derived 部分被切掉！v[0].print() 输出 "Base"，v[0].d 是垃圾值

// 正确：存指针
std::vector<std::unique_ptr<Base>> v2;
v2.push_back(std::make_unique<Derived>(1, 2));
v2[0]->print();  // "Derived" ✅
```

### 三种降低拷贝代价的方法

```cpp
// 1. 存智能指针——拷贝指针 = 拷贝 8 字节
std::vector<std::shared_ptr<Widget>> v;

// 2. emplace 直接构造——不产生临时对象
v.emplace_back(args...);  // 直接在容器内存里构造，省一次拷贝

// 3. 移动语义——转移资源所有权而非拷贝
v.push_back(std::move(obj));  // 移动构造（如果 obj 有移动构造函数）
```

---

## 常见错误（新手踩坑）

### 错误 1：vector<Base> 存 Derived 对象

```cpp
std::vector<Base> v;
v.push_back(Derived{1, 2});
v[0].print();  // 输出 "Base" 而非 "Derived"——对象切片
```

**修正：** 多态对象存指针：`std::vector<std::unique_ptr<Base>>`。

### 错误 2：忘了 push_back 会拷贝

```cpp
std::vector<std::string> v;
std::string s = "hello world this is a long string";
v.push_back(s);  // 拷贝 s（可能堆分配）
// s 仍然可用（push_back 拷贝，不移动）
```

**修正：** 如果不再需要 `s`，用 `v.push_back(std::move(s))` 或 `v.emplace_back("hello...")`。

### 错误 3：扩容时的拷贝尖峰

```cpp
std::vector<BigObject> v;
for (int i = 0; i < 10000; ++i)
    v.push_back(obj);  // 每次扩容都拷贝所有旧元素
```

**修正：** `v.reserve(10000);` 一次性分配。C++11 后扩容用移动而非拷贝（如果有移动构造函数），但仍有一次大批量移动的尖峰。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 插入 | `memcpy` / `=` | 拷贝构造函数 | C++ 有构造/析构 |
| 多态 | 函数指针 | 虚函数 + 存指针 | 对象切片 |
| 拷贝代价 | 固定（sizeof） | 可变（深拷贝/浅拷贝） | 拷贝构造函数可自定义 |
| 优化 | 无 | `emplace` / `move` | 避免不必要的拷贝 |

**一句话：** C 的赋值是固定大小 memcpy，C++ 的拷贝可能触发深拷贝/分配。存指针让拷贝变廉价，`emplace` 省去临时对象，`move` 转移资源。

---

## HFT 关联

- **存指针避免深拷贝**：策略对象（大且复杂）存 `vector<shared_ptr<Strategy>>`，插入只拷贝 16 字节（指针 + 控制块指针）。
- **`emplace_back` 零拷贝**：热路径上 `v.emplace_back(price, qty)` 比 `v.push_back(Order(price, qty))` 省一次临时对象构造+析构。
- **`reserve` + 移动语义**：预 reserve 消除扩容，加上 `push_back(std::move(obj))` 让扩容时的元素迁移走移动而非拷贝。

---

## 代码自测

### Q1: 对象切片
```cpp
class Base { public: int b=0; virtual void f() { puts("B"); } };
class Derived : public Base { public: int d=0; void f() override { puts("D"); } };

std::vector<Base> v;
v.push_back(Derived{1, 2});
v[0].f();  // 输出什么？sizeof(v[0]) 是多少？
```

<details>
<summary>答案</summary>

- 输出 **"B"**（对象切片，vptr 切回 Base）。
- `sizeof(v[0])` = `sizeof(Base)`（Derived 的 `d` 成员被丢弃）。

`vector<Base>` 按 `Base` 大小分配每个槽位，`push_back(Derived)` 调用 `Base(const Base&)` 拷贝构造，只拷贝 Base 部分。
</details>

### Q2: emplace vs push_back
```cpp
struct Order {
    Order(int price, int qty) { puts("construct"); }
    Order(const Order&) { puts("copy"); }
    Order(Order&&) { puts("move"); }
};

std::vector<Order> v;
v.reserve(10);
v.push_back(Order(100, 5));  // A
v.emplace_back(200, 10);      // B
```
> A 和 B 各输出什么？

<details>
<summary>答案</summary>

- **A**：`construct` → `move`（或 `copy`，如果没有移动构造函数）。先构造临时对象，再移动/拷贝进容器。
- **B**：`construct`（直接在容器内存里构造，省去临时对象和移动/拷贝）。

`emplace_back` 把参数转发给 `Order` 构造函数，直接在容器的内存位置构造对象。
</details>

### Q3: 移动语义
```cpp
std::vector<std::string> v;
std::string s = "hello world this is a very long string";
v.push_back(s);              // A
v.push_back(std::move(s));   // B
// B 之后 s 的状态是什么？
```

<details>
<summary>答案</summary>

- **A**：拷贝 `s`，`s` 仍然有效（内容不变）。
- **B**：移动 `s`，`s` 变为有效但未指定状态（通常为空字符串）。移动后不应再使用 `s` 的内容。

移动语义把 `s` 的内部指针转移给容器中的新元素，零拷贝。
</details>

### Q4: 扩容拷贝
```cpp
struct Big { int data[1000]; };
std::vector<Big> v;
for (int i = 0; i < 1000; ++i) v.push_back(Big{});
// vs
std::vector<Big> v2;
v2.reserve(1000);
for (int i = 0; i < 1000; ++i) v2.push_back(Big{});
```
> 两个方案各发生多少次 Big 的拷贝/移动？

<details>
<summary>答案</summary>

- **方案 1（无 reserve）**：约 10 次扩容，每次扩容把所有旧元素移动到新内存。总拷贝/移动次数 ≈ 1000（push_back）+ 1000（扩容迁移，如果用移动）≈ 2000 次。
- **方案 2（有 reserve）**：0 次扩容，只有 1000 次 push_back 拷贝/移动。

`reserve` 消除了扩容时的批量迁移，性能差距可达数倍。
</details>

---

## 参考与延伸

- 上一节：[Item 2 不要写容器无关代码](item02-no-container-agnostic.md)
- 下一节：[Item 4 empty() 而非 size()==0](item04-empty-not-size-zero.md)
- 回到：[第 1 章 容器](README.md)
