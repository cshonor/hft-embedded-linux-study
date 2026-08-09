# 第 6 章 函数

本章全面介绍函数的定义、声明及其高级特性，说明函数作为命名计算单元在程序结构化中的重要作用。

## 小节

- [函数基础与参数传递](./6.1-函数基础与参数传递.md)
- [返回类型](./6.2-返回类型.md)
- [函数重载（Overloaded Functions）](./6.3-函数重载（OverloadedFunctions）.md)
- [特殊用途语言特性](./6.4-特殊用途语言特性.md)
- [函数指针](./6.5-函数指针.md)


## 章节摘要

函数基础（形参/实参/返回值）、参数传递（值传递 vs 引用传递）、`const` 形参、数组形参、返回类型（值返回/引用返回/列表返回）、函数重载、默认实参、内联函数与 `constexpr` 函数、函数指针。

### 和 C 的区别

| C | C++ |
|---|-----|
| 只有值传递 | 有引用传递 `void f(int &x)` |
| 无函数重载 | 可重载（名字相同参数不同） |
| 无默认参数 | `void f(int x, int y = 10)` |
| 函数指针 `void (*fp)(int)` | 同 C，但有 `std::function` |
| `#define` 伪内联 | `inline` 关键字 |
| `setjmp`/`longjmp` | 异常处理 |

## 章节自测

### Q1: 传值 vs 传引用

```cpp
void reset_val(int x)  { x = 0; }
void reset_ref(int &x) { x = 0; }
int main() {
    int n = 42;
    reset_val(n);
    std::cout << n << " ";   // A
    reset_ref(n);
    std::cout << n;          // B
}
```

> A 和 B 分别输出什么？

<details>
<summary>答案与复习指引</summary>

**A: 42**（值传递——`x` 是 `n` 的拷贝，修改 `x` 不影响 `n`）
**B: 0**（引用传递——`x` 是 `n` 的别名，修改 `x` 就是修改 `n`）

**和 C 的区别：** C 只有值传递。要修改实参必须传指针 `void reset(int *x) { *x = 0; }`。C++ 引用更简洁安全（不可空、不可悬垂初始化）。

**复习：** → [函数基础与参数传递](./6.1-函数基础与参数传递.md)
</details>

### Q2: const 引用形参

```cpp
void print(const std::string &s) { std::cout << s; }
int main() {
    print("hello");     // A: 合法吗？
    std::string s = "world";
    print(s);           // B
}
```

> A 行合法吗？`const` 引用有什么好处？

<details>
<summary>答案与复习指引</summary>

**A 行合法。** `const` 引用可以绑定到右值（临时对象）。`"hello"` 是 `const char*`，会隐式构造临时 `std::string`，`const &` 绑定到这个临时对象。

**`const` 引用好处：**
1. 避免拷贝大对象（`string`/`vector` 拷贝昂贵）
2. 保证不修改实参（`const` 契约）
3. 能接受右值（字面量、临时对象）

**非 const 引用不能绑定右值**——`void f(string &s); f("hi");` 编译错误。

**复习：** → [函数基础与参数传递](./6.1-函数基础与参数传递.md)
</details>

### Q3: 函数重载

```cpp
void f(int x)  { std::cout << "int "; }
void f(double x) { std::cout << "double "; }
int main() {
    f(42);       // A
    f(3.14);     // B
    f('a');      // C
    f(42L);      // D
}
```

> A、B、C、D 分别调用哪个重载？

<details>
<summary>答案与复习指引</summary>

- A: `f(int)` — 精确匹配
- B: `f(double)` — 精确匹配
- C: `f(int)` — `char` 提升为 `int`
- D: `f(int)` — `long` 转为 `int`（窄化转换，但有 `int` 重载就选它；如果没有 `int` 只有 `double`，选 `double`）

**和 C 的区别：** C 不支持函数重载——同名函数只能有一个。C++ 通过 name mangling 实现重载。

**复习：** → [函数重载（Overloaded Functions）](./6.3-函数重载（OverloadedFunctions）.md)
</details>

### Q4: 默认实参

```cpp
std::string make_greeting(const std::string &name, const std::string &prefix = "Hello") {
    return prefix + ", " + name + "!";
}
int main() {
    std::cout << make_greeting("World") << " ";
    std::cout << make_greeting("World", "Hi");
}
```

> 输出是什么？默认实参有什么限制？

<details>
<summary>答案与复习指引</summary>

**输出：** `Hello, World! Hi, World!`

**限制：**
1. 默认实参只能从右往左提供——有默认值的参数右边不能有无默认值的参数
2. 在函数声明中指定默认值，定义中不能再指定
3. 同一作用域内不能重复指定默认值

**复习：** → [特殊用途语言特性](./6.4-特殊用途语言特性.md)
</details>

### Q5: 函数指针

```cpp
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int main() {
    int (*fp)(int, int) = add;
    std::cout << fp(3, 4) << " ";
    fp = sub;
    std::cout << fp(3, 4);
}
```

> 输出是什么？函数指针和 C 有什么区别？

<details>
<summary>答案与复习指引</summary>

**输出：** `7 -1`

**和 C 的区别：** 基本相同。C++ 增加了 `std::function`（更灵活，可存 lambda/闭包）和 `auto` 推导（`auto fp = add;`），但底层函数指针机制一致。

**复习：** → [函数指针](./6.5-函数指针.md)
</details>
