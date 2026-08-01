# 条款 6：分清前置 ++/-- 和后置 ++/-- 重载写法、性能差异

## 本节讲什么

自增/自减的前缀（`++i`）与后缀（`i++`）在 C++ 里本是无参同名操作符，语言用**后缀形式的 dummy `int` 参数**区分二者。两者在**返回值类型、效率、实现方式**上差异显著。本条款说明语法约定、为何后缀返回 `const`、性能结论，以及「后缀以前缀实现」的维护准则。

## 1. 语法与参数：后缀形式的 `int` 参数

重载通常靠参数类型区分，但 `++`/`--` 的前缀、后缀原本都无参。C++ 规定：**后缀形式带一个 `int` 类型参数**；调用时编译器自动传入 `0`，与前缀形式区分。

```cpp
class UPInt {
    unsigned int value_;
public:
    UPInt &operator++();       // 前缀：++i
    const UPInt operator++(int);  // 后缀：i++，int 仅为占位，调用时可忽略
    UPInt &operator--();       // 前缀：--i
    const UPInt operator--(int);  // 后缀：i--
};
```

## 2. 返回值类型：前缀返回引用，后缀返回 const 对象

| 形式 | 语义 | 典型返回值 |
|------|------|------------|
| **前缀** `++i` | 先增，再取回 | **引用**（如 `UPInt&`） |
| **后缀** `i++` | 先取回旧值，再增 | **`const` 对象**（如 `const UPInt`） |

### 为何后缀必须返回 `const`？

若后缀返回非 `const` 对象，可写出 `i++++`，等价于：

```cpp
i.operator++(0).operator++(0);
```

第二次 `++` 改的是**第一次返回的临时对象**，不是原始对象 `i`——与内置 `int` 行为不一致，也违背直觉。返回 **`const`** 可禁止对临时对象再次后缀自增。

```cpp
// 若后缀返回非 const：
// UPInt i;
// i++++;  // 第二次 ++ 作用于临时对象，i 只加 1 — 荒谬

// 后缀返回 const → i++++ 无法编译
```

## 3. 效率：优先前缀，除非需要旧值

后缀形式须**保存增前的值**并作为返回值，必然**构造临时对象**，随后还要**析构**——额外开销。前缀直接改对象并返回引用，无此临时对象。

**结论**：若只为「加一」，**尽量用前缀** `++i` / `--i`；只有需要**后缀返回的原值**时才用 `i++` / `i--`。

```cpp
UPInt i;
++i;   // 推荐：无临时对象
// UPInt j = i++;  // 需要旧值时才用后缀
```

对迭代器等「重类型」对象，差异更明显（拷贝/析构成本高）。

## 4. 实现准则：后缀以前缀为基础

前缀与后缀除返回值外，**功能应完全一致**（值加一或减一）。维护准则：

> **后缀操作符应以前缀操作符为基础实现**——只维护前缀版本，后缀内部调用前缀，行为自然一致。

```cpp
class UPInt {
    unsigned int value_;
public:
    UPInt &operator++() {
        ++value_;
        return *this;
    }

    const UPInt operator++(int) {
        UPInt old = *this;  // 保存旧值
        ++(*this);          // 调用前缀版本
        return old;         // 返回 const 临时（类型为 const UPInt）
    }

    UPInt &operator--() {
        --value_;
        return *this;
    }

    const UPInt operator--(int) {
        UPInt old = *this;
        --(*this);
        return old;
    }
};
```

自减 `--` 同理：前缀返回引用，后缀返回 `const` 并按前缀实现。

## 完整示例

```cpp
#include <iostream>

class Counter {
    int v_;
public:
    explicit Counter(int v = 0) : v_(v) {}

    int get() const { return v_; }

    Counter &operator++() {
        ++v_;
        return *this;
    }

    const Counter operator++(int) {
        Counter old = *this;
        ++(*this);
        return old;
    }
};

int main() {
    Counter c(0);

    ++c;
    std::cout << c.get() << '\n';  // 1

    Counter old = c++;             // 需要旧值
    std::cout << old.get() << ' ' << c.get() << '\n';  // 1 2

    // c++++;  // 错误：后缀返回 const，禁止连续后缀
}
```

## 小结

- **区分语法**：后缀重载带 dummy `int` 参数。
- **返回值**：前缀 → 引用；后缀 → **`const` 对象**（禁止 `i++++` 类荒谬用法）。
- **效率**：仅为加一/减一时用**前缀**；后缀有临时对象构造/析构开销。
- **实现**：**后缀调用前缀**，只维护一份核心逻辑。
