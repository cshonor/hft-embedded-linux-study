# 5.1 构造与析构顺序

> 第 5 章 构造、析构与拷贝 · 上一节：[本章导读](README.md) · 下一节：[5.2 存储期与生命周期](02-storage-duration.md)

## 这节讲什么

对象从诞生到销毁的全过程——构造顺序和析构顺序严格对称。构造时 vptr 指向当前类，所以构造函数里调虚函数不表现多态。

---

## 为什么要学这个（先建立直觉）

C 程序员手动调用 init/cleanup，容易忘记或搞错顺序：

```c
// C：手动初始化/清理
struct Device_C {
    int* buffer;
    int fd;
};
void device_init(struct Device_C* d) {
    d->buffer = malloc(1024);  // 先分配 buffer
    d->fd = open("/dev/tty", O_RDWR);  // 再打开 fd
}
void device_cleanup(struct Device_C* d) {
    free(d->buffer);  // 先释放 buffer
    close(d->fd);     // 再关闭 fd
    // 如果忘了调 cleanup → 资源泄漏
}
```

C++ 的构造/析构由编译器自动调用，顺序固定——**RAII 保证资源不泄漏**：

```cpp
class Device {
    int* buffer;  // 成员1
    int fd;       // 成员2
public:
    Device() : buffer(new int[1024]), fd(open("/dev/tty", O_RDWR)) {}
    // 构造顺序：buffer 先（声明序），fd 后
    ~Device() {
        close(fd);      // 析构顺序：fd 先（逆序）
        delete[] buffer; // buffer 后
    }
    // 离开作用域自动调 ~Device()，不会忘
};
```

---

## 顺序规则详解

### 构造顺序

```cpp
class Derived : public Base {
    MemberA a;   // 成员1（声明序）
    MemberB b;   // 成员2
public:
    Derived() { /* 3. 自身构造体 */ }
};
// 构造顺序：
// 1. Base::Base()        — 基类先构造
// 2. a.MemberA::MemberA() — 成员按声明序构造
// 3. b.MemberB::MemberB()
// 4. Derived 构造体执行    — 自身最后
```

### 析构顺序（严格逆序）

```cpp
// 析构顺序：
// 1. ~Derived 析构体执行   — 自身先析构
// 2. b.~MemberB()         — 成员按声明逆序析构
// 3. a.~MemberA()
// 4. ~Base::Base()        — 基类最后析构
```

### vptr 在构造时的变化

```cpp
class Base {
public:
    Base() {
        // 此时 vptr → Base vtable
        // 调虚函数 → 调 Base 版本（不是 Derived）
        log();  // 调 Base::log()
    }
    virtual void log() { cout << "Base"; }
};
class Derived : public Base {
public:
    Derived() {
        // Base 构造完后，vptr → Derived vtable
        // 此时调虚函数 → 调 Derived 版本
        log();  // 调 Derived::log()
    }
    void log() override { cout << "Derived"; }
};
Derived d;
// 输出：BaseDerived（先 Base::log，后 Derived::log）
```

---

## 常见错误（新手踩坑）

### 错误 1：构造函数调虚函数期望多态

```cpp
class Base {
public:
    Base() { init(); }
    virtual void init() { cout << "Base init"; }
};
class Derived : public Base {
public:
    void init() override { cout << "Derived init"; }
};
Derived d;  // 输出 "Base init"（不是 "Derived init"）
```

### 错误 2：析构顺序依赖

```cpp
class Bad {
    Connection* conn;
    Logger* logger;
public:
    ~Bad() {
        conn->close();  // 如果 logger 先析构了，conn->close 可能打日志失败
    }
    // 析构顺序：~Bad 体 → logger 析构 → conn 析构
    // 如果 conn 的析构需要 logger → 坏了（logger 已经析构）
};
```

### 错误 3：忘了虚析构

```cpp
class Base { public: ~Base() {} };  // 非虚析构
class Derived : public Base { int* data; ~Derived() { delete[] data; } };
Base* p = new Derived;
delete p;  // 只调 ~Base()，data 泄漏
```

---

## 和 C 的区别

| 特性 | C init/cleanup | C++ 构造/析构 |
|------|---------------|-------------|
| 调用方式 | 手动调用 | 编译器自动调用 |
| 顺序 | 程序员控制 | 固定（基类→成员→自身，析构逆序） |
| 忘记调用 | 资源泄漏 | 不可能（RAII 保证） |
| vptr | N/A | 构造时指向当前类，析构时也指向当前类 |

---

## HFT 关联

1. **RAII 资源管理**：构造获取资源，析构释放资源——HFT 用 RAII 管 fd/mbuf/锁。`LockGuard lg(mutex);` 离开作用域自动解锁。
2. **构造时 vptr 限制**：构造函数里调虚函数不表现多态——避免在构造函数里依赖多态分派。
3. **析构 noexcept**：析构函数默认 noexcept（C++11 起）——HFT 析构绝不抛异常（否则 terminate 拉崩进程）。

---

## 代码自测

### Q1: 构造析构顺序

```cpp
class Member {
public:
    Member(const char* n) { cout << n << " ctor"; }
    ~Member() { cout << n << " dtor"; }
};
class Base {
    Member m1{"B1"};
public:
    Base() { cout << "Base ctor"; }
    ~Base() { cout << "Base dtor"; }
};
class Derived : public Base {
    Member m2{"D2"};
public:
    Derived() { cout << "Derived ctor"; }
    ~Derived() { cout << "Derived dtor"; }
};
Derived d;
// 构造和析构的输出顺序？
```

<details>
<summary>答案与复习指引</summary>

构造：`B1 ctor → Base ctor → D2 ctor → Derived ctor`（基类→成员→自身）。
析构：`Derived dtor → D2 dtor → Base dtor → B1 dtor`（自身→成员逆序→基类）。严格对称。

**复习：** → [5.1 构造与析构顺序](./01-ctor-dtor-order.md)
</details>

### Q2: 构造函数调虚函数

```cpp
class Base {
public:
    Base() { setup(); }
    virtual void setup() { cout << "Base"; }
};
class Derived : public Base {
public:
    void setup() override { cout << "Derived"; }
};
Derived d;  // 输出什么？
```

<details>
<summary>答案与复习指引</summary>

输出 `Base`。`Base()` 执行时 vptr 指向 Base 的 vtable，`setup()` 调的是 `Base::setup()`。构造函数里调虚函数不表现多态——vptr 在基类构造期间指向基类的 vtable。

**复习：** → [5.1 构造与析构顺序](./01-ctor-dtor-order.md)
</details>

### Q3: 虚析构

```cpp
class Base { public: virtual ~Base() = default; };
class Derived : public Base {
    int* data = new int[100];
public:
    ~Derived() { delete[] data; }
};
Base* p = new Derived;
delete p;  // data 会被释放吗？
```

<details>
<summary>答案与复习指引</summary>

会。`~Base()` 是 virtual，`delete p` 经 vtable 调用，先调 `~Derived()`（释放 data），再调 `~Base()`。如果 `~Base()` 不是 virtual，只调 `~Base()`，data 泄漏。**有虚函数的类必须虚析构。**

**复习：** → [5.1 构造与析构顺序](./01-ctor-dtor-order.md)
</details>

### Q4: 析构顺序陷阱

```cpp
class Engine {
    Logger* logger;    // 声明1
    Connection* conn;  // 声明2
public:
    ~Engine() {
        // conn 的析构需要 logger 打日志
    }
    // 析构顺序：~Engine体 → conn析构 → logger析构
    // 有什么问题？
};
```

<details>
<summary>答案与复习指引</summary>

析构按声明逆序：conn 先析构，logger 后析构。如果 conn 的析构需要 logger（打日志），那 conn 析构时 logger 还活着——没问题。但如果反过来（logger 声明在后，conn 声明在前），logger 先析构，conn 后析构时 logger 已死。**声明顺序要注意依赖关系：被依赖的成员先声明。**

**复习：** → [5.1 构造与析构顺序](./01-ctor-dtor-order.md)
</details>

---

## 参考与延伸

- 下一节：[5.2 存储期与生命周期](02-storage-duration.md)
- 回到：[第 5 章](README.md)
