# 条款 7：多态基类声明 virtual 析构函数

## 本节讲什么

**Declare destructors virtual in polymorphic base classes.** 通过**基类指针** `delete` **派生类对象**时，若基类析构**非 virtual** → **未定义行为（UB）**，典型表现是**只析构基类子对象**（partial destruction）→ 资源泄漏、数据破坏。多态基类必须有 **virtual ~Base()**；**非多态基类不要**为了「保险」乱加 virtual。

← [条款 6：禁用拷贝](./item06-不想用编译器自动生成的函数，就明确禁用.md) · [条款 5](./item05-了解C++默默编写并调用哪些函数.md) 缺省析构 **non-virtual**

---

## 1. 为什么多态基类必须 virtual 析构？

### 典型场景：工厂返回基类指针

```cpp
class TimeKeeper {
public:
    ~TimeKeeper() { /* 非 virtual — 危险 */ }
    // ...
};

class AtomicClock : public TimeKeeper { /* 派生资源 */ };

TimeKeeper* getClockFactory() {
    return new AtomicClock;
}

delete getClockFactory();   // ❌ 若 ~TimeKeeper 非 virtual → UB
```

| 基类析构 | `delete` 基类指针指向派生对象 |
|----------|------------------------------|
| **non-virtual** | UB；常只跑 `~TimeKeeper`，**不跑** `~AtomicClock` → **局部销毁** |
| **virtual** | 先 `~AtomicClock`，再 `~TimeKeeper` → 完整销毁 |

### 准则

> **类里只要有 virtual 函数，几乎就应该有 virtual 析构函数。**

```cpp
class TimeKeeper {
public:
    virtual ~TimeKeeper() = default;
    virtual std::string getUniversalTime() const = 0;
};
```

---

## 2. 为什么不能给所有类都加 virtual 析构？

**不是多态基类**的类，**不要**无故 virtual：

| 代价 | 说明 |
|------|------|
| **vptr** | 每个对象多一个虚表指针，体积增大（32 位下有时接近 **+50%～100%**） |
| **运行时开销** | 析构/部分操作走 vtbl 间接调用 |
| **布局** | 与 C 结构体 / 其他语言 ABI **不兼容**，可移植性变差 |

与 [条款 1](../ch01-accustom-yourself/item01-视C++为一个语言联邦.md) 一致：OOP 多态有成本；**值类型、工具类、不通过基类指针 delete 的类** → **non-virtual 析构**即可。

**两个极端都错：**

- 多态基类 **从不** virtual 析构  
- 普通类 **一律** virtual 析构  

---

## 3. 警惕：不要多态地继承 STL / `std::string`

`std::string`、`vector`、`list`、`set` 等**不是**为多态基类设计的 → **没有 virtual 析构**。

```cpp
// ❌ 反模式
class SpecialString : public std::string { /* ... */ };

std::string* ps = new SpecialString;
delete ps;   // UB：只析构 string 部分，SpecialString 部分泄漏
```

**不要**继承标准容器做「多态扩展」；用 **组合**（成员 `std::string`）或 **模板 / 概念** 扩展行为。

---

## 4. 纯 virtual 析构：打造抽象类

想强制**不可实例化**，又暂时没有别的纯虚函数时，可声明 **纯 virtual 析构**：

```cpp
class AWOV {   // Abstract w/o Virtuals（除析构外无其它虚函数）
public:
    virtual ~AWOV() = 0;   // 纯 virtual
};

// ⚠️ 仍必须在类外提供定义！
AWOV::~AWOV() { /* 通常空 */ }
```

**原因**：派生类析构结束后，编译器**仍要调用基类析构**；纯虚析构**必须有函数体**，否则**链接失败**。

```cpp
class Concrete : public AWOV {
public:
    ~Concrete() override = default;
};
// Concrete c;  // ❌ 抽象类不能实例化
```

---

## 决策表

| 类的角色 | 析构函数 |
|----------|----------|
| **多态基类**（有 virtual 函数、经基类指针 delete） | **`virtual ~Base()`** |
| **抽象基类**（仅禁止实例化） | **`virtual ~Base() = 0`** + **类外定义** |
| **值类型 /  final 具体类 / 不作基类** | **non-virtual** |
| **继承 `std::string` / STL 当多态基类** | **不要这样设计** |

---

## HFT 视角

- **策略接口 / 行情源 / 连接抽象**：工厂返回 `Base*`，必须 **virtual 析构**；否则网关、socket 缓冲在派生析构里泄漏。
- **tick、Order、Price 等 POD/值对象**：**不要** virtual 析构，保持紧凑布局与缓存友好。
- **别** `class MyVec : public std::vector<T>` 再 `vector<T>* p = new MyVec; delete p;` —— 与 SpecialString 同陷阱。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| 有 virtual 函数却无 virtual 析构 | 补上 `virtual ~Base()` |
| 值类型也 virtual 析构「求稳」 | 去掉 virtual，省 vptr |
| 继承 `std::string` / 容器 + 基类指针 delete | 改组合，勿 public 继承 |
| 纯 virtual 析构无定义 | 类外写 `{}` 定义 |

---

## 核心总结

1. **基类指针 delete 派生对象** → 基类析构必须 **virtual**，否则 UB + 局部销毁。
2. **有 virtual 成员 ≈ 要有 virtual 析构**。
3. **非多态类**不要 virtual 析构 —— 体积、ABI、性能。
4. **STL / string** 非多态基类，**禁止**当多态基类继承。
5. **纯 virtual 析构** 可造抽象类，但**必须提供定义**。

← [条款 6：禁用拷贝](./item06-不想用编译器自动生成的函数，就明确禁用.md) | [条款 8：析构与异常 →](./item08-别让异常逃离析构函数.md)
