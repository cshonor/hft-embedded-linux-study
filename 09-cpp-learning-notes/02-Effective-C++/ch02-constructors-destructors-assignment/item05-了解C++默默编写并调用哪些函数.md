# 条款 5：了解 C++ 默默编写并调用哪些函数

## 本节讲什么

**Know what functions C++ silently writes and calls.** 声明一个「空类」时，若你不写特定函数，编译器会在幕后生成**缺省版本**（通常 **public + inline**）。必须清楚：**默认行为是什么、何时会生成、何时会拒绝生成**——否则拷贝/赋值/析构会在你意想不到的地方出错。

← [条款 4：初始化](../ch01-accustom-yourself/item04-确定对象被使用前已先被初始化.md) · 禁用自动生成见 [条款 6](./item06-不想用编译器自动生成的函数，就明确禁用.md)

---

## 空类 ≠ 真的空

```cpp
class Empty {};
```

编译器可能为你生成（在**被需要**时）：

| 函数 | 默认行为概要 |
|------|----------------|
| **Default constructor** | 调用基类 + 各非 static 成员的 default 构造 |
| **Destructor** | 调用基类 + 各非 static 成员的析构；**非 virtual**（除非基类析构已是 virtual） |
| **Copy constructor** | **成员级（member-wise）** 拷贝每个非 static 成员 |
| **Copy assignment `operator=`** | 成员级拷贝赋值（与拷贝构造类似） |

> **「被需要」**：只有代码里真正创建、拷贝、赋值、销毁对象时，编译器才会**实例化**对应函数（链接阶段仍可能用到）。

---

## 1. 四大函数的默认行为

### Default constructor

- 你**没有声明任何构造函数**时，编译器生成 default ctor。
- 一旦你**自己写了任意构造函数**，编译器**不再**自动生成 default ctor（除非 `= default`）。

```cpp
class Widget {
public:
    Widget(int x) : val(x) {}
    // 没有 Widget() — 编译器不会自动生成
};
// Widget w;  // ❌ 可能没有 default ctor
```

### Destructor

- 逆序析构成员、调用基类析构。
- **缺省是非 virtual 的** → 多态基类若缺 virtual 析构会出问题（[条款 7](./item07-多态基类声明virtual析构函数.md)）。

### Copy constructor / Copy assignment

- 对**每个非 static 数据成员**分别拷贝 / 赋值（浅拷贝语义）。
- 成员是指针时 → **两个对象共享同一块堆内存**（双重删除、泄漏），往往要自己写或禁用（[条款 6](./item06-不想用编译器自动生成的函数，就明确禁用.md)、[条款 11–12](./item11-赋值运算符处理自我赋值.md)）。

```cpp
class HasMembers {
    int x;
    std::string s;
    // 编译器生成：逐成员拷贝 s（string 深拷贝），拷贝 x
};
```

---

## 2. 编译器拒绝生成 `operator=` 的情况

默认拷贝赋值在下列情形下**不合法或不合理**，编译器会**拒绝生成**并报错；若要支持赋值，必须**手写** `operator=`：

| 情况 | 原因 |
|------|------|
| **含引用成员** | 引用不能改绑到另一对象 |
| **含 `const` 成员** | 不能给 const 成员赋新值 |
| **基类 `operator=` 为 private** | 派生类生成的 `operator=` 无法调用基类赋值 |

```cpp
class WithRef {
    int& r;
public:
    WithRef(int& x) : r(x) {}
    // 编译器通常不生成 operator= — 无法让 r 改指别的 int
};

class WithConst {
    const int id;
public:
    explicit WithConst(int i) : id(i) {}
    // 编译器不生成 operator= — 不能改 id
};
```

与 [条款 4](../ch01-accustom-yourself/item04-确定对象被使用前已先被初始化.md) 一致：`const`/引用成员**只能初始化、不能赋值**，连带影响编译器版 `operator=`。

### 基类 private `operator=`

```cpp
class Base {
public:
    Base& operator=(const Base&) = delete;  // 或 private 且不实现
};
class Derived : public Base {
    // 编译器生成的 Derived::operator= 需要调 Base::operator=
    // 若基类赋值不可访问 → 拒绝生成 Derived::operator=
};
```

---

## 3. 手写一个函数 → 其它自动生成可能消失

经典组合（Effective C++ 时代 **Rule of Three**）：

| 你若声明了… | 常见后果 |
|-------------|----------|
| 任意构造函数 | **无** default ctor（除非 `= default`） |
| 拷贝构造 | 拷贝赋值可能仍生成，但三/五法则常需一起定制 |
| 析构 | 常暗示资源管理 → 应审视拷贝构造/赋值 |

现代 C++11 起还有 **move 构造 / move 赋值** 的隐式生成规则；与拷贝一样，**资源类**不要 blindly 依赖编译器默认行为。

---

## HFT 视角

- **行情 tick / 订单 POD**：只有 trivial 成员时，编译器生成的拷贝往往够用；一旦含 **`std::string`、`std::vector`、裸指针、文件句柄**，默认 member-wise 可能**浅拷贝**共享缓冲区。
- **单例 / 全局配置**：注意 [条款 4](../ch01-accustom-yourself/item04-确定对象被使用前已先被初始化.md) 的静态初始化；带非 trivial 析构的全局对象还有**析构顺序**问题。
- **含 `const` 订单 ID、引用绑定的外部 buffer**：默认 **不可赋值**——设计时要么禁止拷贝赋值（`= delete`），要么手写语义。

---

## 易错点速查

| 误以为 | 实际 |
|--------|------|
| 空类没有任何函数 | 四大函数可能被隐式生成 |
| 编译器总会生成 `operator=` | 引用/const/基类 private 时会拒绝 |
| 默认析构适合多态基类 | 缺省 **non-virtual** → 条款 7 |
| 写了带参构造还能 `T obj;` | 除非 `= default` 或再写 default ctor |

---

## 核心总结

1. 空类也可能有 **default ctor、dtor、copy ctor、copy assign**——**被需要**时才生成，且多为 **public inline**。
2. 拷贝/赋值默认 **member-wise**；指针/句柄类危险，常需自写或 **= delete**（条款 6）。
3. **引用 / const 成员 / 基类 private operator=** → 编译器**不生成**拷贝赋值。
4. 析构默认 **non-virtual**；多态基类必须 virtual（条款 7）。
5. 写类前先问：**编译器会替我生成什么？够用吗？**

← [条款 4：初始化](../ch01-accustom-yourself/item04-确定对象被使用前已先被初始化.md) | [条款 6：明确禁用 →](./item06-不想用编译器自动生成的函数，就明确禁用.md)
