# 2.3 成员初始化列表

> 第 2 章 · 上一节：[2.2 按位拷贝 vs 逐成员拷贝](02-bitwise-vs-memberwise.md) · 下一节：[2.4 NRVO](04-nrvo.md)

## 这节讲什么

`const` 成员、引用成员、无默认构造的成员对象**必须**在初始化列表初始化。初始化列表比构造体赋值更高效——直接构造，省掉默认构造+赋值两步。

---

## 为什么要学这个（先建立直觉）

C 程序员习惯"先分配，再赋值"：

```c
struct Config_C {
    int port;
    double timeout;
};
void init_config(struct Config_C* c, int p, double t) {
    c->port = p;       // 赋值（内存已分配）
    c->timeout = t;    // 赋值
}
```

C++ 的对象构造分两个阶段：
1. **初始化阶段**：所有成员在这个阶段被初始化（初始化列表在这里生效）
2. **构造体阶段**：`{ }` 里的代码执行（此时成员已经初始化完毕）

```cpp
class Config {
    int port;
    const int id;      // const 成员
    std::string host;  // 成员对象
public:
    // 初始化列表在 { 之前生效
    Config(int p, const std::string& h, int i)
        : port(p), id(i), host(h) {  // 直接构造，1 步
        // 到这里时 port/id/host 都已初始化完毕
    }

    // 对比：在构造体里赋值
    Config(int p, const std::string& h, int i) {
        port = p;          // 赋值（port 已默认初始化为垃圾值，再赋值）
        // id = i;         // 编译错误！const 不能赋值
        host = h;          // 先默认构造 host（空字符串），再赋值（2 步！）
    }
};
```

关键区别：**初始化列表是"直接构造"，构造体赋值是"先默认构造再赋值"——差一步。**

---

## 三种必须用初始化列表的情况

### 1. const 成员

```cpp
class Connection {
    const int id;  // const → 必须初始化，不能赋值
public:
    Connection(int i) : id(i) {}  // ✓ 初始化列表
    // Connection(int i) { id = i; }  // ✗ 编译错误！const 不能赋值
};
```

### 2. 引用成员

```cpp
class View {
    Buffer& buf;  // 引用 → 必须在初始化时绑定
public:
    View(Buffer& b) : buf(b) {}  // ✓
    // View(Buffer& b) { buf = b; }  // ✗ 引用不能重新绑定
};
```

### 3. 无默认构造的成员对象

```cpp
class Port {
public:
    Port(int num);  // 只有带参构造，没有默认构造
};
class Device {
    Port p;  // Port 没有默认构造
public:
    Device(int portNum) : p(portNum) {}  // ✓ 必须在列表里构造
    // Device(int portNum) { p = Port(portNum); }  // ✗ p 无默认构造，无法先默认构造
};
```

---

## 常见错误（新手踩坑）

### 错误 1：初始化顺序依赖

```cpp
class Bad {
    int a;
    int b;
public:
    Bad(int val) : b(val), a(b + 1) {}  // 看起来 a = b + 1 = val + 1
    // 实际：a 先初始化（声明序），此时 b 还没初始化 → a = 垃圾值 + 1
};
// 修正：Bad(int val) : a(val), b(a + 1) {}  // 按声明序
```

### 错误 2：在构造体里赋值导致两次构造

```cpp
class Inefficient {
    std::string name;
public:
    Inefficient(const std::string& n) {
        name = n;  // 先默认构造 name（空串），再 operator= 赋值 → 2 步
    }
    // 高效：Inefficient(const std::string& n) : name(n) {}  // 直接构造 → 1 步
};
```

### 错误 3：忘了基类构造

```cpp
class Base { public: Base(int x) {} };  // 基类只有带参构造
class Derived : public Base {
public:
    Derived(int x) : Base(x) {}  // ✓ 必须在列表调基类构造
    // Derived(int x) {}  // ✗ 编译错误！Base 没有默认构造
};
```

---

## 和 C 的区别

| 特性 | C 初始化 | C++ 初始化列表 |
|------|---------|--------------|
| 时机 | 定义时赋值 | 进入构造体之前 |
| const 成员 | N/A（C 没有 const 成员限制） | **必须**用初始化列表 |
| 引用成员 | N/A（C 没有引用） | **必须**用初始化列表 |
| 成员对象 | N/A（C 没有构造函数） | **必须**用初始化列表（如果无默认构造） |
| 效率 | N/A | 直接构造 vs 先默认构造再赋值 |
| 执行顺序 | 按代码顺序 | **按声明顺序**（不是列表书写顺序！） |

---

## HFT 关联

1. **热路径配置对象用列表**：成员对象用初始化列表省一次默认构造 + 赋值。`Order(symbol, qty, price)` 用 `: symbol_(symbol), qty_(qty), price_(price) {}` 比 `{ symbol_ = symbol; ... }` 快。
2. **成员声明顺序 = 初始化顺序**：把热路径成员（频繁访问的）放前面，cold member 放后面——改善 cache 局部性（前面的成员更可能在同一 cache 行）。
3. **避免构造体赋值**：在 HFT 代码审查中，构造体里的 `member = value` 是性能 anti-pattern——应该改成初始化列表。

---

## 代码自测

### Q1: 初始化顺序

```cpp
class Widget {
    int a;
    int b;
public:
    Widget(int val) : b(val), a(b + 1) {}
    // a 和 b 的值分别是什么？
};
Widget w(10);
```

<details>
<summary>答案与复习指引</summary>

`a` = 垃圾值 + 1（未定义行为），`b` = 10。初始化按**声明顺序**（先 a 后 b），不是列表书写顺序。`a` 初始化时 `b` 还没被初始化。修正：`Widget(int val) : a(val), b(a + 1) {}`。

**复习：** → [2.3 成员初始化列表](./03-init-list.md)
</details>

### Q2: 必须用列表的情况

```cpp
class Port {
public:
    Port(int num);  // 只有带参构造
};
class Device {
    const int id;
    Port& ref;
    Port port;
    std::string name;
};
// Device 的构造函数必须怎么写？
```

<details>
<summary>答案与复习指引</summary>

```cpp
Device(int i, Port& r, int portNum, const std::string& n)
    : id(i), ref(r), port(portNum), name(n) {}
```
`id` 是 const → 必须列表。`ref` 是引用 → 必须列表。`port` 无默认构造 → 必须列表。`name` 有默认构造，但列表更高效。

**复习：** → [2.3 成员初始化列表](./03-init-list.md)
</details>

### Q3: 效率对比

```cpp
class A {
    std::string s;
public:
    A(const std::string& str) { s = str; }        // 方式1
};
class B {
    std::string s;
public:
    B(const std::string& str) : s(str) {}         // 方式2
};
// 哪个更高效？为什么？
```

<details>
<summary>答案与复习指引</summary>

方式2更高效。方式1：先默认构造 `s`（空字符串，分配/空指针），再 `operator=` 赋值（可能重新分配）→ 2 步。方式2：直接用 `str` 构造 `s` → 1 步，省掉默认构造 + 赋值的开销。

**复习：** → [2.3 成员初始化列表](./03-init-list.md)
</details>

### Q4: 基类初始化

```cpp
class Base {
    int x;
public:
    Base(int val) : x(val) {}
};
class Derived : public Base {
    int y;
public:
    Derived(int a, int b) : y(b) {}  // 有什么问题？
};
```

<details>
<summary>答案与复习指引</summary>

编译错误。`Derived` 的初始化列表没调 `Base(a)`，而 `Base` 没有默认构造。修正：`Derived(int a, int b) : Base(a), y(b) {}`。

**复习：** → [2.3 成员初始化列表](./03-init-list.md)
</details>

---

## 参考与延伸

- 下一节：[2.4 NRVO](04-nrvo.md)
- 回到：[第 2 章 构造函数语义](README.md)
