# 第 13 章 拷贝控制

本章介绍当类的对象发生拷贝、移动、赋值和销毁时，类如何通过特殊成员函数控制这些行为。

## 小节

- [拷贝控制成员](./13.1-拷贝控制成员.md)
- [资源管理与三/五法则](./13.2-资源管理与三五法则.md)
- [对象移动（C++11）](./13.3-对象移动（C++11）.md)
- [其他应用](./13.4-其他应用.md)


## 章节摘要

拷贝控制五个特殊成员函数：拷贝构造、拷贝赋值、移动构造、移动赋值、析构。三/五法则、右值引用与移动语义、`std::move`、引用限定符。

### 和 C 的区别

| C | C++ |
|---|-----|
| `struct` 按位拷贝 | 可自定义拷贝行为（深拷贝） |
| 无移动语义 | `std::move` + 移动构造（O(1) 资源转移） |
| 无 RAII | 析构自动释放资源 |
| 无值语义 | 对象可按值传递/返回（自动拷贝/移动） |

## 章节自测

### Q1: 拷贝构造 vs 移动构造

```cpp
class Buffer {
    int *data;
    size_t size;
public:
    Buffer(size_t n) : data(new int[n]), size(n) {}
    ~Buffer() { delete[] data; }
    // 拷贝构造（深拷贝）
    Buffer(const Buffer &o) : data(new int[o.size]), size(o.size) {
        std::copy(o.data, o.data + size, data);
    }
    // 移动构造（资源转移）
    Buffer(Buffer &&o) noexcept : data(o.data), size(o.size) {
        o.data = nullptr; o.size = 0;
    }
};
Buffer make_buf() { return Buffer(1000); }
Buffer b = make_buf();  // 调用哪个构造？
```

> `b = make_buf()` 调用拷贝还是移动？如果没有移动构造呢？

<details>
<summary>答案与复习指引</summary>

**调用移动构造。** `make_buf()` 返回右值（纯右值），优先匹配移动构造（`Buffer&&`）。

**如果没有移动构造：** 调用拷贝构造——C++11 之前就是这样。移动构造是 C++11 新增的优化，把 O(n) 拷贝降为 O(1) 指针交换。

**`noexcept` 的重要性：** 标记 `noexcept` 后，STL 容器（如 `vector`）扩容时会用移动而非拷贝。未标 `noexcept` 的移动构造，STL 退回拷贝（保证强异常安全）。

**复习：** → [对象移动（C++11）](./13.3-对象移动（C++11）.md)
</details>

### Q2: 三五法则

```cpp
class Resource {
    int *data;
public:
    Resource(int n) : data(new int(n)) {}
    ~Resource() { delete data; }
    // 没写拷贝构造和拷贝赋值
};
Resource a(42);
Resource b = a;  // 会发生什么？
```

> `b = a` 后会发生什么？什么是三五法则？

<details>
<summary>答案与复习指引</summary>

**double free / 崩溃。** 编译器生成的默认拷贝构造是**按位拷贝**——`b.data` = `a.data`（指向同一块内存）。`a` 和 `b` 析构时都 `delete` 同一块内存 → double free。

**三五法则：** 如果你需要自定义以下任何一个，就应该全部定义（或 `=delete`/`=default`）：
1. 拷贝构造
2. 拷贝赋值
3. 析构
4. 移动构造（C++11）
5. 移动赋值（C++11）

**根因：** 需要自定义析构（释放资源）意味着类管理资源，默认的按位拷贝会出问题（浅拷贝 → double free）。

**复习：** → [资源管理与三/五法则](./13.2-资源管理与三五法则.md)
</details>

### Q3: std::move 不移动

```cpp
std::string s = "hello";
std::string r = std::move(s);
// s 现在是什么状态？
std::cout << s;  // 合法吗？
```

> `s` 在 `std::move` 后是什么状态？读取 `s` 合法吗？

<details>
<summary>答案与复习指引</summary>

**`s` 处于"有效但未指定"状态。** `std::move` 只是一个 `static_cast<string&&>`——它不移动任何东西。真正移动发生在 `string` 的移动构造函数里（把 `s` 的内部指针转移给 `r`）。

**读取 `s` 合法但值未指定**——可能是空字符串，也可能有旧值（取决于实现）。标准保证对象处于有效状态（可析构、可赋值），但不保证具体值。

**教训：** `std::move` 后不要使用原对象的值（但可以赋新值或析构）。

**复习：** → [对象移动（C++11）](./13.3-对象移动（C++11）.md)
</details>

### Q4: 赋值运算符自赋值

```cpp
class String {
    char *data;
    size_t len;
public:
    String &operator=(const String &other) {
        delete[] data;                    // A: 先释放自己的
        data = new char[other.len + 1];   // B: 分配新的
        strcpy(data, other.data);
        len = other.len;
        return *this;
    }
};
String s("hello");
s = s;  // 自赋值：会出什么问题？
```

> 自赋值时会发生什么？如何修复？

<details>
<summary>答案与复习指引</summary>

**问题：** `this == &other` 时，A 行 `delete[] data` 释放了自己的数据，B 行 `other.data` 已经是悬垂指针——`strcpy` 读已释放内存 = UB。

**修复方案：**
1. **身份检查**：`if (this == &other) return *this;`
2. **先拷贝再释放**：先 `new` + `strcpy` 到临时指针，成功后再 `delete[]` 旧 `data`，再赋值
3. **copy-and-swap**：`String &operator=(String other) { swap(*this, other); return *this; }` — 按值传参自动拷贝，swap 交换，旧数据在 `other` 析构时释放

**复习：** → [拷贝控制成员](./13.1-拷贝控制成员.md)
</details>
