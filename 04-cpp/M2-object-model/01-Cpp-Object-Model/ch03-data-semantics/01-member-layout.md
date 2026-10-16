# 3.1 成员布局规则

> 第 3 章 数据语义 · 上一节：[本章导读](README.md) · 下一节：[3.2 继承布局](02-inheritance-layout.md)

## 这节讲什么

类的数据成员在内存中如何排列？padding 如何影响 `sizeof`？这是预测 cache 行为的基础——字段排列直接影响一个 cache 行能装几个对象。

---

## 为什么要学这个（先建立直觉）

C 程序员对结构体布局已经很熟悉——padding 和对齐在 C 里一样存在：

```c
// C：结构体 padding
struct Bad_C {
    char c;    // 1 字节 + 3 字节 padding
    int i;     // 4 字节
    char c2;   // 1 字节 + 3 字节 padding
};
// sizeof = 12，浪费了 6 字节 padding

struct Good_C {
    int i;     // 4 字节
    char c;    // 1 字节
    char c2;   // 1 字节 + 2 字节 padding
};
// sizeof = 8，只浪费 2 字节
```

C++ 的 class 成员布局规则和 C struct **完全一样**——但 C++ 多了 vptr、继承、访问控制等复杂因素。核心规则不变：**大对齐类型放前面，小对齐放后面，减少 padding。**

---

## 核心规则详解

### 规则 1：非 static 成员按声明顺序排列

```cpp
struct A {
    char c;     // offset 0
    int i;      // offset 4（c 后 3 字节 padding）
};
// sizeof(A) = 8
```

### 规则 2：padding 按对齐要求插入

```cpp
struct B {
    int i;      // offset 0, 4 字节
    char c;     // offset 4, 1 字节
};
// sizeof(B) = 8（c 后 3 字节 padding，保证数组 B[2] 的第二个元素对齐）
```

### 规则 3：整体对齐 = 最大成员的对齐

```cpp
struct C {
    char c;     // 1 字节
    double d;   // 8 字节对齐 → c 后 7 字节 padding
};
// sizeof(C) = 16
```

### 规则 4：static 成员不在对象内

```cpp
struct D {
    int x;
    static int count;  // 不占对象空间
};
// sizeof(D) = 4
```

### 字段重排减 padding

```cpp
// 差：padding 多
struct BadOrder {
    char a;      // 1 + 7 padding
    double b;    // 8
    char c;      // 1 + 3 padding
    int d;       // 4
};
// sizeof = 24

// 好：padding 少
struct GoodOrder {
    double b;    // 8
    int d;       // 4
    char a;      // 1
    char c;      // 1 + 2 padding
};
// sizeof = 16
```

---

## 常见错误（新手踩坑）

### 错误 1：char 和 int 交替排列

```cpp
struct Inefficient {
    char a; int b; char c; int d; char e;
};
// sizeof = 20（每个 char 后都有 padding）
// 修正：int b; int d; char a; char c; char e; → sizeof = 12
```

### 错误 2：忘了数组对齐

```cpp
struct Item {
    char tag;   // 1 + 3 padding
    int value;  // 4
};
// sizeof = 8
Item arr[100];
// arr[1].value 在 offset 8（对齐）—— 但如果 sizeof 是 5（无 padding），
// arr[1].value 就在 offset 5（不对齐）—— 所以尾部 padding 是必要的
```

### 错误 3：位域的对齐陷阱

```cpp
struct Flags {
    uint8_t a : 3;
    uint8_t b : 5;
    uint32_t c : 1;  // 跨边界？编译器可能加 padding
};
// 位域布局因编译器而异，不可移植
```

---

## 和 C 的区别

| 特性 | C struct | C++ class |
|------|----------|-----------|
| 成员排列 | 声明顺序 | 声明顺序（相同） |
| padding | 按对齐规则 | 相同 |
| static 成员 | 无（用全局变量） | 不在对象内 |
| 访问控制 | 无（全 public） | 不影响布局（private/public 混合排列） |
| 位域 | C 有 | 相同（但跨编译器不可移植） |

**C++ 独有**：vptr（8B）在对象头部（如果有虚函数），影响所有成员的 offset。

---

## HFT 关联

1. **字段重排减 padding**：合理排列成员（大对齐在前）减少 sizeof，cache 友好。`struct Tick { double price; int qty; int symbol; char side; }` 比 `struct Tick { char side; double price; int qty; int symbol; }` 少 8 字节。
2. **cache 行利用率**：64 字节 cache 行能装 `64 / sizeof(对象)` 个。sizeof 从 40 减到 32，每行从 1 个变 2 个——吞吐翻倍。
3. **热路径结构体审计**：HFT 代码审查时检查热路径结构体的字段排列——用 `__attribute__((packed))` 或手动重排优化。

---

## 代码自测

### Q1: sizeof 推断

```cpp
struct A { char c; int i; };
struct B { int i; char c; };
// sizeof(A) = ?  sizeof(B) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(A) = 8`（char 1 + padding 3 + int 4）。`sizeof(B) = 8`（int 4 + char 1 + padding 3）。sizeof 相同！但 B 的排列更好——如果后面还有 char 成员，B 可以复用 padding 空间。

**复习：** → [3.1 成员布局规则](./01-member-layout.md)
</details>

### Q2: 字段重排

```cpp
struct Bad {
    char a;      // 1
    double b;    // 8
    char c;      // 1
    int d;       // 4
    char e;      // 1
};
// sizeof = ?  如何重排？
```

<details>
<summary>答案与复习指引</summary>

`sizeof = 32`（a 1 + pad 7 + b 8 + c 1 + pad 3 + d 4 + e 1 + pad 7）。重排：`double b; int d; char a; char c; char e;` → sizeof = 16（b 8 + d 4 + a 1 + c 1 + e 1 + pad 1）。

**复习：** → [3.1 成员布局规则](./01-member-layout.md)
</details>

### Q3: static 成员

```cpp
struct Counter {
    int id;
    static int total;
    double rate;
};
// sizeof(Counter) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof = 16`（int 4 + padding 4 + double 8）。`total` 是 static，不在对象内，不占空间。

**复习：** → [3.1 成员布局规则](./01-member-layout.md)
</details>

### Q4: cache 行优化

```cpp
struct Tick_Bad {
    char side;       // 1
    char status;     // 1
    double price;    // 8
    int qty;         // 4
    long timestamp;  // 8
};
// sizeof = ?  一个 64B cache 行能装几个？
```

<details>
<summary>答案与复习指引</summary>

sizeof = 32（side 1 + status 1 + pad 6 + price 8 + qty 4 + pad 4 + timestamp 8）。cache 行装 64/32 = 2 个。重排为 `double price; long timestamp; int qty; char side; char status;` → sizeof = 24 → cache 行装 64/24 = 2 个（但浪费 16 字节）。或 `long timestamp; double price; int qty; char side; char status;` → sizeof = 24，同上。

**复习：** → [3.1 成员布局规则](./01-member-layout.md)
</details>

---

## 参考与延伸

- 下一节：[3.2 继承布局](02-inheritance-layout.md)
- 回到：[第 3 章 数据语义](README.md)
