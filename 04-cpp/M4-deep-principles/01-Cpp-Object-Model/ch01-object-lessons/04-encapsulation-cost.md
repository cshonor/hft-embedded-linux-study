# 1.4 封装的代价

> 第 1 章 · 上一节：[1.3 继承布局](03-inheritance-layout.md) · 下一章：[第 2 章 构造函数语义](../ch02-constructor-semantics/README.md)

## 这节讲什么

C++ 的封装（`private`/`public`）运行时零开销——访问控制是编译期检查。真正的代价来自虚函数、虚基类、多继承。这是 C++ "零开销原则"的核心体现。

---

## 为什么要学这个（先建立直觉）

C 程序员没有访问控制——struct 的所有成员都是 public，谁都能改：

```c
// C：没有访问控制
struct BankAccount_C {
    long balance;  // 任何人都能直接改！
};
// 没法阻止 account->balance = 0;  // 直接清零
```

Java/C# 程序员有访问控制，但所有方法默认虚函数——运行时总有代价。C++ 的设计哲学是：**你不使用的东西，你不需要付出代价**：

```cpp
// C++：访问控制是编译期检查，运行时零开销
class BankAccount {
    long balance;       // private：编译期阻止直接访问
public:
    void deposit(long amt) { balance += amt; }  // 运行时和直接改 balance 一样快
};
// account.balance = 0;  // 编译错误！
// account.deposit(100); // 编译后等价于 account.balance += 100
```

关键洞察：**`private`/`public` 只在编译期检查——编译通过后，运行时没有任何额外开销。** `account.deposit(100)` 和直接写 `account.balance += 100` 生成的机器码一样。

---

## 零开销原则详解

### 哪些特性是零开销的

```cpp
class Widget {
private:                    // 编译期检查 → 运行时零开销
    int data;
public:
    int get() const { return data; }  // 内联后零开销
};
// Widget w; w.get() 编译后等价于直接读 w.data
```

### 哪些特性有运行时代价

```cpp
class Widget {
    virtual void f();       // vtable 间接 → 有代价
};
// -fno-exceptions          // 异常表 → 可关
// -fno-rtti               // RTTI → 可关
```

### 对比 Java

| 特性 | C++ | Java |
|------|-----|------|
| 访问控制 | 编译期，零运行时开销 | 运行时检查（部分） |
| 方法分派 | 默认直接 call（可内联） | 默认虚分派（不可内联） |
| 异常 | 零开销模型（正常路径无代价） | 异常总有开销 |
| GC | 无（手动/RAII） | 有（STW 暂停） |

---

## 常见错误（新手踩坑）

### 错误 1：以为 private 有运行时代价

```cpp
class Data {
private:
    int secret;
public:
    int getSecret() const { return secret; }
};
// 新手以为 getSecret() 有函数调用开销
// 实际：内联后 getSecret() 等价于直接读 secret，零开销
```

### 错误 2：用了虚函数以为和普通函数一样快

```cpp
class Strategy {
public:
    virtual int run() { return 42; }  // 虚函数
    int runFast() { return 42; }      // 普通函数
};
// s->run()     → 间接 call，不可内联
// s->runFast() → 直接 call（或内联），快
```

### 错误 3：忘了关异常/RTTI

```cpp
// 不用异常也不用 RTTI，但编译时没关：
// g++ -O2 code.cpp
// 二进制里仍含异常表和 RTTI 信息
// 修正：g++ -O2 -fno-exceptions -fno-rtti code.cpp
```

---

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 访问控制 | 无（全 public） | `private`/`public`/`protected`，编译期检查 |
| 封装开销 | N/A | **零**（编译期检查，运行时无区别） |
| 不用的特性 | N/A | **零开销**（不用虚函数 = 无 vtable） |
| 哲学 | 最小化运行时 | "不用的东西不付代价" |

---

## HFT 关联

1. **零开销是 HFT 选 C++ 的根本原因**：不用虚函数 = 无 vtable 开销；`-fno-exceptions` = 无异常开销；`-fno-rtti` = 无 RTTI 开销。只付用到的特性的代价。
2. **访问控制不影响性能**：可以把所有热路径字段设为 private + 提供 inline getter/setter——和直接访问一样快，但更安全。
3. **编译选项组合**：`-O2 -fno-exceptions -fno-rtti` 是 HFT 常见编译组合，最小化运行时开销。

---

## 代码自测

### Q1: 访问控制开销

```cpp
class A {
    int x;                    // private
public:
    int get() const { return x; }
};
class B { public: int x; };   // 全 public

A a; a.get();   // 编译后等价于读 a.x
B b; b.x;       // 直接读 b.x
// 两者生成的机器码有区别吗？
```

<details>
<summary>答案与复习指引</summary>

没有区别（开了优化后）。`A::get()` 会被内联，生成的机器码和直接读 `a.x` 完全一样。`private` 只是编译期检查，运行时零开销。

**复习：** → [1.4 封装的代价](./04-encapsulation-cost.md)
</details>

### Q2: 零开销原则

```cpp
class X { void f() {} };                          // A：无虚函数
class Y { virtual void f() {} };                  // B：有虚函数
class Z { virtual void f() {} };  // 编译时 -fno-rtti  // C：有虚函数但关RTTI
// sizeof(X) = ?  sizeof(Y) = ?  sizeof(Z) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(X) = 1`（空类，无虚函数）。`sizeof(Y) = 8`（有 vptr）。`sizeof(Z) = 8`（-fno-rtti 只移除 type_info，不移除 vptr——虚函数分派仍需 vtable）。

**复习：** → [1.4 封装的代价](./04-encapsulation-cost.md)
</details>

### Q3: 编译选项

```bash
# 哪个编译选项组合最适合 HFT 热路径？
# A: g++ -O0 -g code.cpp
# B: g++ -O2 code.cpp
# C: g++ -O2 -fno-exceptions -fno-rtti code.cpp
```

<details>
<summary>答案与复习指引</summary>

C 最适合。`-O2` 开优化（内联、向量化等），`-fno-exceptions` 移除异常表（减小二进制 + 确定性），`-fno-rtti` 移除运行时类型信息（减小二进制）。前提是代码确实不用异常和 RTTI。

**复习：** → [1.4 封装的代价](./04-encapsulation-cost.md)
</details>

### Q4: C++ vs Java

> 为什么 HFT 选 C++ 而不是 Java？从零开销原则角度解释。

<details>
<summary>答案与复习指引</summary>

Java 的方法默认虚分派（不可内联），有 GC 暂停（STW），异常总有开销。C++ 的零开销原则保证：不用虚函数 = 无 vtable 开销；不用异常 = 无异常表；不用 GC = 无 STW。HFT 需要微秒级确定性延迟，C++ 能精确控制每一纳秒。

**复习：** → [1.4 封装的代价](./04-encapsulation-cost.md)
</details>

---

## 参考与延伸

- 下一章：[第 2 章 构造函数语义](../ch02-constructor-semantics/README.md)
- 回到：[第 1 章 关于对象](README.md)
