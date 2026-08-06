# 条款 4：确定对象被使用前已先被初始化

## 本节讲什么

**Make sure that objects are initialized before they're used.** 读未初始化值 → **未定义行为（UB）**：可能崩溃、错价、静默数据损坏。从三层处理：**内置类型手动初始化**、**成员初始化列表**、**跨编译单元静态对象顺序**。

← 第一章收尾；下一章 [条款 5](../ch02-constructors-destructors-assignment/item05-了解C++默默编写并调用哪些函数.md) 继续构造相关规则。

---

## 1. 内置类型：始终手动初始化

C++ **不保证**非成员的内置类型对象会被自动初始化（C 子语言遗留）。对 **`int`、`double`、指针** 等，**离开作用域前必须显式赋初值**：

```cpp
int x = 0;              // 好
double price = 0.0;
int* p = nullptr;       // 指针也要初始化

void f() {
    int local;          // ❌ 未定义：栈上 int 不保证为 0
    int local = 0;      // ✅
}
```

| 存储期 | 内置类型默认行为（典型） |
|--------|--------------------------|
| 全局 / namespace 作用域 | 零初始化 |
| 栈上局部 | **不初始化**（UB 若先读后用） |
| 堆 `new int` | **不初始化**；`new int()` 值初始化 |

**HFT**：热路径局部变量、协议解析临时量、计数器——一律显式初始化，别赌编译器「碰巧是 0」。

---

## 2. 成员初始化列表（Member Initialization List）

自定义类型的初始化由**构造函数**负责。关键区分：**赋值（assignment）≠ 初始化（initialization）**。

### 构造函数体内「赋值」的代价

```cpp
class PhoneNumber { /* ... */ };

class Person {
    std::string name;
    PhoneNumber phone;
public:
    Person(const std::string& n, const PhoneNumber& p) {
        name = n;       // 先 default 构造 name，再 operator=
        phone = p;        // 先 default 构造 phone，再 operator=
    }
};
```

每个成员被 **default 构造一次 + 赋值一次**——多出来的构造/析构浪费，且有些类型根本不能默认构造后再赋。

### 推荐：初始化列表直接构造

```cpp
Person(const std::string& n, const PhoneNumber& p)
    : name(n)           // 直接拷贝构造
    , phone(p)          // 直接拷贝构造
{}
```

列表中的实参直接驱动成员的**拷贝/移动构造**，通常更高效。

### 必须用初始化列表的情况

**`const` 成员**和**引用成员**不能赋值，**只能初始化**：

```cpp
class ConstRefHolder {
    const int id;
    std::string& alias;
public:
    ConstRefHolder(int i, std::string& s)
        : id(i)         // 必须
        , alias(s)      // 必须
    {}
};
```

与 [条款 3](./item03-尽可能使用const.md) 一致：`const` 数据成员只此一条路。

### 初始化顺序陷阱

C++ **按类内声明顺序**初始化成员，**与初始化列表书写顺序无关**：

```cpp
class Bad {
    int a;
    int b;
public:
    Bad(int x) : b(x), a(b) {}  // ❌ 先 init a（用未初始化的 b）
};
```

**规范**：初始化列表顺序 **= 成员声明顺序**。

### 优先用对象而非裸指针

能 **`std::string`、`std::vector`、值类型成员** 就不要 `char*` / 手动 `new`——成员随对象构造自动初始化，减少漏初始化（示例见原书 `PhoneNumber` / `Person`）。

---

## 3. 跨编译单元：非局部静态对象的初始化顺序

### 问题（Static Initialization Order Fiasco）

多个 **translation unit（.cpp）** 各自有 **非局部静态对象**（全局、namespace 作用域、`static` 全局）时：

- 若 A.cpp 里某静态对象初始化**依赖** B.cpp 里另一个静态对象；
- **C++ 不规定**不同 TU 之间非局部静态对象的**相对初始化顺序**；

→ 启动阶段可能用到**尚未初始化**的对象 → UB。

```cpp
// file1.cpp
std::string& getLogFile() {
    static std::string log = computePath();  // 局部 static
    return log;
}

// file2.cpp — ❌ 危险：全局对象初始化时可能早于 file1 的 static
std::ofstream out(getLogFile());  // 若写成全局直接依赖另一 TU 的 static...
```

### 解决方案：函数内 local static + 返回引用

**不要**直接暴露跨 TU 依赖的非局部 static；改为：

```cpp
FileSystem& tfs() {                    // 每个 static 一个 accessor
    static FileSystem fs;              // 局部 static：首次执行到此处时初始化
    return fs;
}

// 使用方通过函数拿引用，调用顺序由你控制
FileSystem& fs = tfs();
```

**保证**：函数**第一次**被调用且执行流到 `static` 定义行时，该对象**一定**完成初始化（C++11 起线程安全的一次性初始化）。

这就是 **Meyers Singleton** 的常见写法，用来**用调用顺序替代不可控的全局初始化顺序**。

| 做法 | 风险 |
|------|------|
| 多 TU 非局部 `static` 互相依赖 | 初始化顺序未定义 |
| `func()` 内 `static T obj; return obj;` | 首次调用时初始化，顺序可控 |

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| 栈上 `int x;` 直接用 | 声明时赋初值 |
| 构造函体内给成员「赋值」 | 改用初始化列表 |
| `const`/引用成员未在列表初始化 | 编译期报错或非法——必须在列表 |
| 列表顺序与声明顺序不一致 | 对齐声明顺序 |
| 全局对象依赖另一 .cpp 的全局对象 | 改为函数内 local static + 引用 |

---

## 核心总结

1. **内置类型**：非成员、尤其局部变量 —— **手动初始化**。
2. **类成员**：用 **初始化列表** 真正构造；`const`/引用 **必须** 列表；顺序跟**声明**走。
3. **跨 .cpp 静态对象**：避免非局部 static 互相依赖；**local static + 返回引用** 控制首次初始化时机。
4. 一句话：**能初始化就别赋值；能延迟到函数内 static 就别赌全局谁先谁后。**

← [条款 3：尽可能使用 const](./item03-尽可能使用const.md) | [第二章：条款 5 →](../ch02-constructors-destructors-assignment/item05-了解C++默默编写并调用哪些函数.md)
