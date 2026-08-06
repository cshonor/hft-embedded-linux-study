# Item 1：理解模板类型推导

> **所属章节：** [ch01 类型推导](./README.md) · Items 1–4 的第 1 条
> **难度：** ⭐⭐⭐（M1 硬门槛，必须吃透）
> **一句话：** 模板推导有三种形态，按值传递会剥掉顶层 const。

---

## 这节讲什么

模板形如 `template<class T> void f(ParamType param)`，调用 `f(expr)` 时编译器要干两件事：
1. 根据 `expr` 推导 `T` 的类型
2. 根据 `ParamType` 的形态（引用？万能引用？按值？）调整最终类型

**问题在于：`T` 的推导结果不仅取决于 `expr`，还取决于 `ParamType` 长什么样。** 同一个实参，`ParamType` 换个写法，`T` 就不一样。这是读 Modern C++ 模板代码（muduo 回调、DPDK C++ 封装）的前提。

---

## 三种 ParamType 形态

### 形态一：ParamType 是引用（`T&`）

```cpp
template<class T>
void f(T& param);

int x = 27;
const int cx = x;
const int& rx = x;

f(x);    // T 推为 int，param 类型是 int&
f(cx);   // T 推为 const int，param 类型是 const int&
f(rx);   // T 推为 const int，param 类型是 const int&（引用性被忽略）
```

**规则：** `expr` 的引用性被忽略，`const` 被保留进 `T`。

**新手要点：** `rx` 虽然是 `const int&`，但推导时引用性被剥掉，`T` 推成 `const int`（不是 `const int&`）。`param` 的引用是 `ParamType` 自己加的。

### 形态二：ParamType 是万能引用（`T&&`）

```cpp
template<class T>
void f(T&& param);

int x = 27;
f(x);    // x 是左值 → T 推为 int&，param 类型是 int& && → 折叠成 int&
f(27);   // 27 是右值 → T 推为 int，param 类型是 int&&
```

**规则：**
- 左值实参 → `T` 推为 `T&`（引用折叠后 `param` 是 `T&`）
- 右值实参 → `T` 推为 `T`（`param` 是 `T&&`）

**新手要点：** `T&&` 在模板里不一定是右值引用！它能接左值也能接右值，所以叫"万能引用"。这是完美转发的根基（Item 25 会详谈）。**判断标准：** 只有当类型推导发生（模板 + `T&&`）时才是万能引用，`void f(int&&)` 这种没有推导的不是。

### 形态三：ParamType 是按值（`T`）

```cpp
template<class T>
void f(T param);  // 按值传递

int x = 27;
const int cx = x;
const char* const ptr = "hello";  // 顶层 const（指针本身）+ 底层 const（指向的字符）

f(x);    // T 推为 int
f(cx);   // T 推为 int（顶层 const 被剥掉！）
f(ptr);  // T 推为 const char*（顶层 const 剥掉，底层 const 保留）
```

**规则：** 按值传递会剥掉**顶层 const** 和引用性，但保留**底层 const**。

**新手要点：** 这是最容易踩的坑——传 `const int` 给按值形参，`T` 是 `int` 不是 `const int`。很多人在泛型代码里发现"const 怎么没了"，根因就在这。

**顶层 vs 底层 const 速记：**
| 表达式 | 顶层 const | 底层 const |
|--------|-----------|-----------|
| `const int x` | ✅（x 本身不可变） | — |
| `const int* p` | ❌（p 可变） | ✅（指向的 int 不可变） |
| `int* const p` | ✅（p 本身不可变） | ❌ |
| `const int* const p` | ✅ | ✅ |

---

## 数组/函数实参的退化

除非 `ParamType` 是引用，否则数组退化为指针、函数退化为函数指针：

```cpp
const char name[] = "hello";  // const char[6]

template<class T> void f(T param);
f(name);  // T 推为 const char*（数组退化）

template<class T> void f(T& param);
f(name);  // T 推为 const char (&)[6]（引用不退化）
```

这个退化规则和 C 完全一致（见《C 和指针》ch08）。**应用：** 用引用形参可以写出"推导数组长度"的模板：

```cpp
template<class T, size_t N>
constexpr size_t arraySize(T (&)[N]) noexcept { return N; }
```

---

## 新手要点（和 C 的区别）

1. **C 没有引用**：C 的函数参数全是按值（指针也是按值），所以 C 程序员看到 `T&` 会懵。记住：引用是 C++ 的"别名"，不是指针。
2. **顶层 const 被剥**：C 程序员习惯了 `void f(int x)` 里 `x` 是副本，`const` 没意义。C++ 模板按值传递同理，`T` 不会带顶层 const。
3. **万能引用不是右值引用**：`T&&` 在模板里是万能引用，在非模板里（`void f(int&&)`）是右值引用。看有没有类型推导来区分。
4. **引用折叠**：`T& &`、`T& &&`、`T&& &` 都折叠成 `T&`，只有 `T&& &&` 折叠成 `T&&`。这个规则决定了万能引用接左值时变成左值引用。

---

## HFT 关联

- **万能引用 + 完美转发**是 muduo / DPDK C++ 封装里回调注册的基石：`template<class F> void set_cb(F&& f)` 用万能引用避免不必要的拷贝。不理解第三种形态，读不懂这类接口。
- **按值传参剥 const** 影响 HFT 里"不可变配置"的传递：如果用模板按值传 `const Config`，`T` 是 `Config` 不是 `const Config`，函数内能改副本——这通常没问题，但要意识到。
- **数组长度推导**：HFT 里固定大小缓冲区（如行情快照数组）可以用 `arraySize` 模板在编译期拿长度，零运行开销。

---

## 自测题

1. `template<class T> void f(T param);` 调用 `f(cx)`（`cx` 是 `const int`），`T` 推导成什么？为什么顶层 const 消失？
2. `template<class T> void f(T&& param);` 分别调用 `f(x)`（左值）和 `f(27)`（右值），`T` 和 `param` 的类型分别是什么？
3. `template<class T> void f(T& param);` 调用 `f(rx)`（`rx` 是 `const int&`），`T` 推导成什么？引用性为什么被忽略？
4. `const char* const p` 传给按值形参 `T param`，`T` 是什么？顶层和底层 const 分别怎么处理？
5. 写一个模板函数，能在编译期推导出 C 风格数组的长度。

---

## 参考与延伸

- **本书关联：** Item 2（auto 推导几乎等同模板）、Item 24（万能引用 vs 右值引用）、Item 25（`move` vs `forward`）
- **C 语言对照：** 《C 和指针》ch08——数组退化为指针的规则
- **下一节：** [Item 2：理解 auto 类型推导](./item02-auto-type-deduction.md)
