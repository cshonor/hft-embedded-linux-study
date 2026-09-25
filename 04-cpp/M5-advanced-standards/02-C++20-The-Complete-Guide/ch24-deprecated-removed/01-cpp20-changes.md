# C++20 弃用与移除

## 弃用的特性

```cpp
// 1. volatile 的很多操作被弃用
volatile int x = 0;
// ++x;  // C++20 弃用 volatile 的复合赋值
// x = x + 1;  // 弃用 volatile 的赋值表达式结果
// 简单赋值 x = 1 仍可用

// 2. 下标运算符中的逗号表达式
// arr[1, 2]  // C++20 弃用（等价 arr[(1,2)] = arr[2]）
arr[1];  // 正确

// 3. noexcept 类型相关的隐式转换放宽
```

## 移除的特性

```cpp
// C++20 移除的（从 C++17 弃用的）：

// 1. char8_t 的隐式转换
// C++17：u8"hello" 返回 const char*
// C++20：u8"hello" 返回 const char8_t*，不能隐式转 const char*
const char* s = u8"hello";  // C++17 OK，C++20 ❌

// 2. 一些 C 标准库弃用部分
// <ccomplex>、<cstdalign>、<cstdbool>、<ctgmath> 移除
```

## 废弃的库组件

```cpp
// C++20 弃用
std::is_literal_type      // 已在 C++17 弃用，C++20 移除
std::result_of            // 已在 C++17 弃用，C++20 移除
std::iterator             // C++17 弃用，C++20 移除

// C++20 新弃用
std::atomic<T>::is_always_lock_free 的某些用法
std::to_address 的某些边缘情况
```

## 迁移影响

```cpp
// 1. u8 字符串处理
// C++17:
const char* s = u8"hello";
std::string str = u8"hello";

// C++20:
const char8_t* s = u8"hello";
// 需要转换：
std::string str(reinterpret_cast<const char*>(u8"hello"));
// 或用 std::u8string
std::u8string u8s = u8"hello";

// 2. volatile 限制
volatile int v = 0;
v = 1;           // ✅ 简单赋值
// v += 1;       // ⚠️ 弃用
int tmp = v;
v = tmp + 1;     // ✅ 手动读写
```

## 自测题

1. C++20 对 `volatile` 的复合赋值做了什么？
2. `u8"hello"` 在 C++17 和 C++20 的类型有什么变化？
3. C++20 移除了哪些 C++17 弃用的库组件？
4. C++20 中 `const char* s = u8"hello"` 会怎样？
5. 迁移到 C++20 时 `volatile` 代码怎么改？

<details>
<summary>参考答案</summary>

1. C++20 **弃用**了 volatile 的若干操作（P1152）：
   - **复合赋值**（`v += 1`、`v -= 1`、`v <<= 1` 等）；
   - **自增/自减**（`++v`、`v++`、`--v`、`v--`）；
   - 函数**参数与返回类型**上多余的顶层 `volatile` 限定；
   - 对 volatile 类型的**结构化绑定**。
**没有**被弃用的是对 volatile 的**简单读与简单赋值**（`v = 1;`、`int t = v;`）——普通访问仍然完全合法（volatile 在内存映射 I/O、信号处理里的用途不变）。
被弃用的理由是这些复合操作把"读-改-写"拆成了多次访问，语义容易被误解，也妨碍优化；显式拆成"读 → 改 → 写"能让真实访问次数一目了然。
2. 类型变了：
   - **C++17**：`u8"hello"` 的类型是 `const char[6]`（即 `const char*` 兼容），和普通的窄字符串字面量没有区别。
   - **C++20**：`u8"hello"` 的类型是 **`const char8_t[6]`**（对应 `const char8_t*`），并配套新增了 `std::u8string`。
这是为了在类型系统里把 UTF-8 文本与"任意字节/本地编码"的窄文本区分开，让重载、推导和编码转换能正确识别 UTF-8。
3. 主要来自 C++17（或更早）被弃用的组件，常见清单包括：
   - `std::result_of`（C++17 弃用 → C++20 移除，改用 `std::invoke_result`）；
   - `std::is_literal_type`（C++17 弃用 → C++20 移除）；
   - `std::iterator` 基类（C++17 弃用 → C++20 移除，改为直接写五个 typedef）；
   - `std::shared_ptr::unique()`；
   - `std::get_temporary_buffer` / `std::return_temporary_buffer`；
   - `std::unary_negate` / `binary_negate` 与 `std::not1` / `std::not2`；
   - 四个 C 兼容头：`<ccomplex>`、`<cstdalign>`、`<cstdbool>`、`<ctgmath>`；
   - `throw()`（空的动态异常规范）——C++17 起等价于 `noexcept(true)` 但已弃用，C++20 移除；
   - `std::u8` 字符串字面量到 `const char*` 的隐式转换（类型变为 `char8_t` 后即失效）。
（确切清单以 cppreference 的"移除的特性"列表为准。）
4. **编译错误**。因为 C++20 里 `u8"hello"` 的类型是 `const char8_t[6]`（→ `const char8_t*`），而 `char8_t` 是独立类型，**不能隐式转换**成 `const char*`。
写法要改成以下之一：
```cpp
const char8_t* s  = u8"hello";             // 用 char8_t
std::u8string u8s = u8"hello";             // 用 u8string
std::string str(reinterpret_cast<const char*>(u8"hello"));  // 确实需要 char 时显式转换
```
C++17 下这行是合法的（那时 `u8` 就是 `const char*`），所以这是迁移 C++20 时最常见的编译错误之一。
5. 按"拆成显式读-改-写"的思路改：
```cpp
volatile int v = 0;
v = 1;              // ✅ 简单赋值，仍合法
// v += 1;          // ⚠️ C++20 弃用
int tmp = v;        // 显式读
tmp = tmp + 1;
v = tmp;            // 显式写
```
迁移要点：
   1. 把复合赋值（`+=`、`-=`、`|=`、`<<=` 等）和 `++`/`--` 全部改写成"读 → 计算 → 写"三步；
   2. 去掉函数参数/返回类型上多余的顶层 `volatile`；
   3. 不要对 volatile 对象做结构化绑定；
   4. 打开弃用警告（GCC/Clang `-Wdeprecated`，MSVC 相应警告）扫一遍；
   5. 若某处确实需要"原子读改写"的语义，说明它本来就不该用 `volatile`——改用 `std::atomic`（这才是正确的工具）。

</details>
