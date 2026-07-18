# 条款 10：令 operator= 返回 *this 引用

## 本节讲什么

**Have assignment operators return a reference to *this.** 自定义 **`operator=`** 以及 **`+=`、`-=`、`*=`** 等复合赋值，应在末尾 **`return *this;`**，使行为与内建类型、标准库一致，并支持 **赋值链** 与 **右结合**。

← [条款 9：构造/析构与虚函数](./item09-绝不在构造和析构过程调用虚函数.md) · 自我赋值见 [条款 11](./item11-赋值运算符处理自我赋值.md)

---

## 1. 赋值链与右结合律

内建类型与标准库都支持：

```cpp
int x, y, z;
x = y = z = 15;
```

**右结合** → 编译器解析为：

```cpp
x = (y = (z = 15));
```

1. `z = 15` → 表达式值是 **`z` 的引用**（左值）  
2. `y = (上一步结果)` → 值是 **`y` 的引用**  
3. `x = …` → 完成链式赋值  

要让**自定义类**也能 `a = b = c`，每个 `operator=` 必须返回 **左侧对象** 的引用；在成员函数里，左操作数就是 **` *this`**。

```cpp
Widget a, b, c;
a = b = c;   // 需要 Widget::operator= 返回 Widget&
```

若返回 `void` 或按值返回 → **`a = b = c` 无法编译** 或语义怪异。

---

## 2. 标准写法

### 拷贝赋值

```cpp
class Widget {
public:
    Widget& operator=(const Widget& rhs) {
        // ... 复制成员（条款 11：自我赋值；条款 12：复制每一个成员）
        return *this;
    }
};
```

### 复合赋值（同样返回 `*this`）

```cpp
Widget& operator+=(const Widget& rhs) {
    // ...
    return *this;
}
Widget& operator-=(int delta) {
    // ...
    return *this;
}
```

| 运算符 | 是否 `return *this` |
|--------|---------------------|
| `operator=` | ✅ |
| `operator+=` / `-=` / `*=` / `/=` … | ✅ |
| 仅 `void operator=(...)` | ❌ 破坏链式与惯例 |

### 参数类型不是本类时

```cpp
Widget& operator=(int rhs) {
    // ...
    return *this;   // 仍返回 *this，不是 int&
}
```

左操作数始终是 **`Widget` 对象**（`*this`），与右操作数类型无关。

---

## 3. 为何只是惯例却必须遵守？

- **不返回引用也能编译** —— 但客户无法写 `w1 = w2 = w3`。
- **内建类型**、`std::string`、`vector`、`complex`、`shared_ptr` 等 **一律** 返回左操作数引用。
- 你的类若返回 `void` / 按值 → API **违反最小惊奇原则**，与生态不一致。

> **除非有极充分理由，不要破坏这一约定。**

C++11 起 **`operator=(T&&)`** 移动赋值同样 **`return *this`**。

---

## 完整签名习惯（与后续条款配合）

```cpp
class Widget {
public:
    Widget& operator=(const Widget& rhs) {       // 拷贝
        if (this == &rhs) return *this;            // 条款 11
        // 复制每一个成员 …                        // 条款 12
        return *this;                              // 条款 10
    }
    Widget& operator=(Widget&& rhs) noexcept {     // 移动（C++11）
        // ...
        return *this;
    }
};
```

非成员 `operator=`（如 [条款 24](../ch04-design-declaration/item24-需要所有参数都支持隐式类型转换时，使用非成员函数.md) 的 `Rational`）通常返回 **`Rational&`**（左操作数那一侧的引用），思路相同。

---

## HFT 视角

- **订单/持仓聚合对象**（`Position& operator+=` 成交量）链式更新：`posA = posB = snapshot` 或 `book += delta1 += delta2` 时，返回 `*this` 与 STL 数值习惯一致，减少 API 惊讶。
- 热路径更常显式分步赋值；但 **public 运算符仍应遵循惯例**，方便测试代码与配置 DSL。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| `operator=` 返回 `void` | 改 `Widget&` + `return *this` |
| 复合赋值忘记 return | 末尾 `return *this` |
| 返回 `const Widget&` | 一般返回 **非 const** `Widget&`（与内建类型一致，允许链上下一步继续赋值） |
| 移动赋值返回 void | 同拷贝，返回 `*this` |

---

## 核心总结

1. 赋值 **右结合** → 子表达式必须返回 **左操作数引用**。
2. 成员 `operator=` 左操作数是 **`*this`** → **`return *this`**。
3. **所有复合赋值** 同样返回 `*this`。
4. 与内建类型、STL **保持一致**；无充分理由不破坏惯例。

← [条款 9：构造/析构与虚函数](./item09-绝不在构造和析构过程调用虚函数.md) | [条款 11：自我赋值 →](./item11-赋值运算符处理自我赋值.md)
