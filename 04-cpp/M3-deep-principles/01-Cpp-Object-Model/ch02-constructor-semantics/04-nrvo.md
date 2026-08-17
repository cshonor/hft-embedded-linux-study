# 2.4 NRVO（命名返回值优化）

> 第 2 章 · 上一节：[2.3 成员初始化列表](03-init-list.md) · 下一章：[第 3 章 数据语义](../ch03-data-semantics/README.md)

## 这节讲什么

编译器消除返回值的拷贝——`Widget make() { Widget w; return w; }` 的 `w` 直接在调用者栈上构造，零拷贝。C++17 起对返回纯右值是强制拷贝省略。在构造体里 `return std::move(w)` 反而会阻止 NRVO。

---

## 为什么要学这个（先建立直觉）

C 程序员返回大对象时用指针避免拷贝：

```c
// C：用输出参数避免拷贝
void make_config(struct Config* out) {
    out->port = 8080;
    out->timeout = 1.0;
}
Config cfg;
make_config(&cfg);  // 直接写入 cfg，零拷贝
```

C++ 程序员想用"值返回"更自然，但担心拷贝开销：

```cpp
// C++：值返回，看起来会拷贝
Config make_config() {
    Config cfg;
    cfg.port = 8080;
    cfg.timeout = 1.0;
    return cfg;  // 看起来拷贝了 cfg 给调用者
}
Config result = make_config();  // 看起来又拷贝了一次
```

NRVO 让上面的代码**零拷贝**——`cfg` 直接在 `result` 的内存上构造，没有中间拷贝。编译器通过在调用者栈上预留空间实现。

---

## 核心机制详解

### NRVO 的工作原理

```cpp
Widget make() {
    Widget w;       // w 不在 make() 的栈上，而在调用者的栈上
    w.setup();
    return w;       // 没有"拷贝 w 到返回值"——w 就是返回值
}
Widget result = make();
// 实际发生：
// 1. 调用者在自己的栈帧里预留 Widget 大小的空间
// 2. make() 的 w 直接在这块空间上构造
// 3. make() 返回时不需要拷贝——result 就是 w
```

### C++17 强制拷贝省略

```cpp
Widget make() {
    return Widget();   // 返回纯右值（prvalue）
}
Widget result = make();
// C++17 前：编译器"可以"省略拷贝（但不是保证）
// C++17 起：标准"保证"省略——不可能有拷贝
```

### URVO（未命名返回值优化）

```cpp
Widget make() {
    return Widget(42);  // 返回临时对象
}
// 即使 C++17 前，主流编译器也做 URVO
// C++17 起保证
```

---

## 常见错误（新手踩坑）

### 错误 1：return std::move(w) 阻止 NRVO

```cpp
Widget make() {
    Widget w;
    w.setup();
    return std::move(w);  // ← 新手以为 move 更高效
    // 实际：move 把 w 变成右值引用，编译器不再能做 NRVO
    // 结果：触发移动构造（如果有）或拷贝构造
}
// 修正：return w;  // 让编译器做 NRVO
```

### 错误 2：多返回路径阻止 NRVO

```cpp
Widget make(int type) {
    Widget a, b;
    if (type == 1) return a;  // 返回 a
    return b;                  // 返回 b
    // NRVO 可能失败——编译器不知道返回 a 还是 b
    // 不能把两个变量都放在调用者栈上
}
// 修正：只返回一个变量
Widget make(int type) {
    Widget result;
    if (type == 1) result = a;
    else result = b;
    return result;  // NRVO 可以生效
}
```

### 错误 3：以为 move 返回总是更快

```cpp
std::string make() {
    std::string s = "hello";
    return std::move(s);  // 比 return s 更慢！
    // return s：NRVO，零拷贝
    // return std::move(s)：阻止 NRVO，触发移动构造（虽快但非零）
}
```

---

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 返回大对象 | 用指针/输出参数 | 值返回 + NRVO（零拷贝） |
| 拷贝省略 | N/A | NRVO（编译器优化） + C++17 强制省略 |
| `return std::move(w)` | N/A | **阻止 NRVO**——新手常见错误 |
| 移动语义 | N/A | `return std::move(w)` 触发移动（比拷贝快，但比 NRVO 慢） |

---

## HFT 关联

1. **工厂函数靠 NRVO**：返回大对象的工厂函数靠 NRVO 消除拷贝。`Order makeOrder() { Order o; o.setSymbol("AAPL"); return o; }` ——零拷贝。
2. **绝不 `return std::move(w)`**：在 HFT 代码审查中，`return std::move(局部变量)` 是性能 anti-pattern——阻止 NRVO，触发不必要的移动。
3. **C++17 强制省略**：`return Widget();` 在 C++17 保证零拷贝——可以放心返回临时对象。

---

## 代码自测

### Q1: NRVO 判断

```cpp
std::vector<int> makeData() {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    return v;  // 会发生拷贝吗？
}
auto data = makeData();
```

<details>
<summary>答案与复习指引</summary>

不会拷贝（NRVO 生效）。`v` 直接在 `data` 的内存上构造，`return v` 时没有拷贝。前提是编译器支持 NRVO（主流编译器 -O1+ 都支持）。

**复习：** → [2.4 NRVO](./04-nrvo.md)
</details>

### Q2: move 阻止 NRVO

```cpp
std::string make() {
    std::string s = "hello world";
    return std::move(s);
}
auto result = make();
// 和 return s 相比，性能如何？
```

<details>
<summary>答案与复习指引</summary>

更差。`return std::move(s)` 把 `s` 变成右值引用，阻止 NRVO，触发 `string` 的移动构造（需拷贝指针 + 清空源对象）。`return s` 让 NRVO 生效，零拷贝。**永远不要 `return std::move(局部变量)`。**

**复习：** → [2.4 NRVO](./04-nrvo.md)
</details>

### Q3: 多返回路径

```cpp
Widget make(int type) {
    if (type == 0) {
        Widget a;
        a.setType(0);
        return a;
    } else {
        Widget b;
        b.setType(1);
        return b;
    }
}
// NRVO 会生效吗？如何改？
```

<details>
<summary>答案与复习指引</summary>

NRVO 可能失败（编译器不知道返回 a 还是 b，不能都放在调用者栈上）。修正：用一个变量：`Widget result; if (type == 0) result.setType(0); else result.setType(1); return result;`

**复习：** → [2.4 NRVO](./04-nrvo.md)
</details>

### Q4: C++17 强制省略

```cpp
Widget make() {
    return Widget(42);  // 返回纯右值
}
Widget w = make();
// C++14 和 C++17 的行为有何不同？
```

<details>
<summary>答案与复习指引</summary>

C++14：编译器"可以"省略拷贝（URVO），但不保证——理论上可能有拷贝。C++17：标准"保证"省略——不可能有拷贝，`Widget(42)` 直接在 `w` 上构造。C++17 的保证让代码更可预测。

**复习：** → [2.4 NRVO](./04-nrvo.md)
</details>

---

## 参考与延伸

- 下一章：[第 3 章 数据语义](../ch03-data-semantics/README.md)
- 回到：[第 2 章 构造函数语义](README.md)
