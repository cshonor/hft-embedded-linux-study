# 第 16 章 模板与泛型编程

本章探讨泛型编程的基础——**模板**，使开发者可编写独立于特定类型的通用代码。

## 小节

- [定义模板](./16.1-定义模板.md)
- [模板参数与成员模板](./16.2-模板参数与成员模板.md)
- [模板实参推断](./16.3-模板实参推断.md)
- [高级特性](./16.4-高级特性.md)


## 章节摘要

模板与泛型编程：函数模板、类模板、非类型模板参数、模板实参推断、可变参数模板、完美转发。

### 和 C 的区别

| C | C++ |
|---|-----|
| `void*` + 函数指针泛型 | 模板（类型安全 + 编译期生成） |
| 宏 `#define MAX(a,b)` | `template<class T> T max(T a, T b)` |
| 无泛型容器 | `vector<T>`/`map<K,V>` |
| `__VA_ARGS__` | 可变参数模板 `template<class... Args>` |

## 章节自测

### Q1: 函数模板推导

```cpp
template<typename T>
T add(T a, T b) { return a + b; }
int main() {
    std::cout << add(1, 2);      // A
    std::cout << add(1, 2.0);    // B
    std::cout << add<double>(1, 2.0);  // C
}
```

> B 编译成功吗？A 和 C 分别输出什么？

<details>
<summary>答案与复习指引</summary>

**B: 编译错误。** `add(1, 2.0)` 中 `T` 从 `1` 推导为 `int`，从 `2.0` 推导为 `double`——冲突，推导失败。

**A: 3** — `T = int`
**C: 3** — 显式指定 `T = double`，`1` 隐式转换为 `double`

**教训：** 模板推导不做隐式类型转换。如果两种类型不同，要显式指定模板参数或用两个类型参数。

**复习：** → [定义模板](./16.1-定义模板.md)
</details>

### Q2: 类模板

```cpp
template<typename T, size_t N>
class Array {
    T data[N];
public:
    T& operator[](size_t i) { return data[i]; }
    constexpr size_t size() const { return N; }
};
Array<int, 5> arr;
arr[0] = 42;
std::cout << arr.size() << " " << arr[0];
```

> 输出是什么？非类型模板参数 `N` 是什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `5 42`

**非类型模板参数 `N`：** 模板参数可以是值而非类型。`N` 必须是编译期常量（`size_t N=5`）。`std::array` 就是这样实现的。

**和 C 宏的区别：** 模板是类型安全的——`Array<int, 5>` 和 `Array<double, 5>` 是不同类型，编译器分别生成代码。C 的宏只是文本替换，无类型检查。

**复习：** → [定义模板](./16.1-定义模板.md)
</details>

### Q3: 可变参数模板

```cpp
template<typename T>
void print(T t) { std::cout << t << std::endl; }
template<typename T, typename... Args>
void print(T t, Args... rest) {
    std::cout << t << " ";
    print(rest...);
}
int main() {
    print(1, "hello", 3.14, 'x');
}
```

> 输出是什么？`Args...` 是什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `1 hello 3.14 x`

**`Args...`：** 参数包（parameter pack），可以接受任意数量、任意类型的参数。递归展开：每次取第一个参数 `T t` 输出，剩余 `rest...` 递归调用 `print`，直到只剩一个参数走基础版本。

**和 C 的 `va_list` 区别：** 模板参数包是**类型安全**的——编译器知道每个参数的类型。C 的 `printf` 用 `va_list` 不检查类型，格式符和参数不匹配是 UB。

**复习：** → [高级特性](./16.4-高级特性.md)
</details>

### Q4: 完美转发

```cpp
template<typename T>
void wrapper(T &&arg) {
    target(std::forward<T>(arg));
}
// 如果传入左值 lval：T 推导为？arg 类型是？
// 如果传入右值 rval：T 推导为？arg 类型是？
```

> 左值和右值分别推导出什么？`std::forward` 的作用是什么？

<details>
<summary>答案与复习指引</summary>

- 传入左值 `lval`（`int&`）：`T` 推导为 `int&`，`arg` 类型为 `int&`（引用折叠 `int& &&` → `int&`），`forward<int&>(arg)` 转发为左值
- 传入右值 `rval`（`int&&`）：`T` 推导为 `int`，`arg` 类型为 `int&&`，`forward<int>(arg)` 转发为右值

**`std::forward<T>` 的作用：** 有条件地转换——当原始实参是右值时转成右值（允许移动），是左值时保持左值。这叫"完美转发"——转发函数保留了原始实参的左右值性。

**`T&&` 在模板中是万能引用**（不是纯右值引用），能同时接受左值和右值。

**复习：** → [高级特性](./16.4-高级特性.md)
</details>
