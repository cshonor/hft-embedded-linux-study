# 条款 3：尽可能使用 const

## 本节讲什么

**只要可能就用 `const`。** 它声明一种语义约束——某对象或某视图**不应被修改**——并由编译器强制执行。对读代码的人，也是在说：「这个值保持不变。」

与 [条款 2](./item02-尽量以const、enum、inline替换#define.md) 衔接：用 `const` 取代宏常量；本条把 `const` 用到指针、迭代器、参数、返回值和成员函数上。

---

## 1. 指针与 STL 迭代器

### 指针：`const` 位置决定含义

| 声明 | 含义 |
|------|------|
| `const T* p` / `T const* p` | 所指**数据**不可改（`const T*`） |
| `T* const p` | **指针本身**不可改 |
| `const T* const p` | 两者都不可改 |

口诀：**`const` 在 `*` 左边 → 数据 const；在 `*` 右边 → 指针 const。**

### 迭代器：别和指针搞混

STL 迭代器以指针为原型，但 `const` 写法容易误读：

```cpp
std::vector<int> v;

// ❌ 常见误解：以为「不能改元素」
const std::vector<int>::iterator it = v.begin();
// 实际是 T* const：it 不能改指向，但 *it 仍可改

// ✅ 要「只读元素」→ const_iterator（类似 const T*）
std::vector<int>::const_iterator cit = v.cbegin();
// 或 v.begin() 配合 cbegin()/cbegin 语义
```

| 写法 | 类比 | 能否改指向 | 能否改元素 |
|------|------|------------|------------|
| `const iterator` | `T* const` | 否 | 是 |
| `const_iterator` | `const T*` | 是（++） | 否 |

Primer 详见 [3.4.2 begin/end 与常量迭代器](../../01-C++Primer/ch03-strings-vectors-arrays/3.4-introducing-iterators/3.4.2-begin-end与常量迭代器.md)。

---

## 2. 函数返回值与参数

### 返回值：拦截无意义赋值

对**按值返回**的类型，加 `const` 可在**不牺牲效率**的前提下，挡住客户误用：

```cpp
class Rational { /* ... */ };

const Rational operator*(const Rational& lhs, const Rational& rhs) {
    return Rational(lhs.n * rhs.n, lhs.d * rhs.d);
}

Rational a, b, c;
// (a * b) = c;           // ❌ 编译错误：不能给 const 返回值赋值
// if (a * b = c) { ... } // ❌ 把 == 写成 = 也会被拦住（返回值是 const）
```

> 注意：返回 **`const` 引用** 往往限制过死（例如链式调用），按值返回的 `const` 对象才是本条典型场景。非成员 `operator*` 还可配合 [条款 24](../ch04-design-declaration/item24-需要所有参数都支持隐式类型转换时，使用非成员函数.md)。

### 参数：默认加 `const`

除非明确要改参数，否则声明为 **`const`**——多打几个字符，能避免意外修改：

```cpp
void print(const std::string& msg);   // 好
void transform(std::string& msg);     // 只有真要改时才非 const
```

与 [条款 20](../ch04-design-declaration/item20-优先使用const引用传参，而非值传递.md) 一致：**const 引用** = 只读 + 少拷贝。

---

## 3. const 成员函数

### 目的

- 标明：**可被 `const` 对象调用**；
- 接口一眼分清「会改对象 / 不会改对象」；
- 支撑 **`const T&` 传参**——只读访问大对象时不拷贝（性能前提）。

**const 与 non-const 成员函数可重载**（仅常量性不同）：

```cpp
class TextBlock {
public:
    const char& operator[](std::size_t pos) const {
        return text[pos];
    }
    char& operator[](std::size_t pos) {
        return text[pos];
    }
private:
    std::string text;
};
```

### 位常量性 vs 逻辑常量性

| 概念 | 含义 |
|------|------|
| **位常量性（bitwise constness）** | 编译器强制：`const` 成员函数**不能改任何非 static 数据成员** |
| **逻辑常量性（logical constness）** | 设计上「对外状态不变」，但内部可能更新缓存、锁、计数等 |

客户关心逻辑状态；编译器只认位状态。需要「`const` 函数里改一点点内部书keeping」时，用 **`mutable`**：

```cpp
class CachedText {
public:
    std::size_t length() const {
        if (!lengthValid) {
            cachedLength = computeLength();  // 逻辑上仍 const
            lengthValid = true;
        }
        return cachedLength;
    }
private:
    std::size_t computeLength() const;
    mutable std::size_t cachedLength = 0;
    mutable bool lengthValid = false;
};
```

**HFT**：只读查询接口（如 `best_bid()`、`size()`）标 `const`；内部 lazy 缓存、统计计数用 `mutable`，热路径仍走 const 引用传参。

---

## 4. const / non-const 重载：避免重复实现

两版逻辑几乎相同（边界检查、日志、校验）时，**不要复制粘贴**。

**正确做法：non-const 版本调用 const 版本**（两次 cast）：

```cpp
class TextBlock {
public:
    const char& operator[](std::size_t pos) const {
        /* 边界检查、日志、真正访问 — 只写一份 */
        return text[pos];
    }

    char& operator[](std::size_t pos) {
        return const_cast<char&>(
            static_cast<const TextBlock&>(*this)[pos]
        );
    }
private:
    std::string text;
};
```

1. `static_cast<const TextBlock&>(*this)` — 调用 **const** 重载；
2. `const_cast<char&>` — 剥掉返回值的 const，得到可写引用。

**禁止反向**：用 const 版去调 non-const 版——non-const 不保证不改对象，会破坏 const 对象的承诺。

---

## 易错点速查

| 坑 | 正确 |
|----|------|
| `const iterator` 当只读遍历 | 用 `const_iterator` / `cbegin` |
| 能改的参数不加 const | 默认 `const&` / `const*` |
| const 与 non-const 各写一整份逻辑 | non-const 委托 const + cast |
| 缓存字段导致 const 函数编译失败 | `mutable` 表达逻辑 const |

---

## 核心总结

1. **`const` = 语义 + 编译器执法**；能加就加（指针、迭代器、参数、返回值、成员函数）。
2. 迭代器：**只读元素用 `const_iterator`**，不是 `const iterator`。
3. 返回值 **`const` 按值对象** 可防 `(a*b)=c`、误写 `=`。
4. 成员函数：**位 const** 由编译器保证；**逻辑 const** 用 **`mutable`**。
5. 重载维护：**non-const 调 const**，绝不反过来。

← [条款 2：const 替换 #define](./item02-尽量以const、enum、inline替换#define.md) | [条款 4：初始化 →](./item04-确定对象被使用前已先被初始化.md)
