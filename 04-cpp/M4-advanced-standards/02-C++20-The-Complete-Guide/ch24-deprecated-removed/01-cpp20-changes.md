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
