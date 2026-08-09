# 第 24 章 已弃用与移除的特性

**Deprecated and Removed Features**

## 本章讲什么

C++20 弃用和移除的特性。迁移到 C++20 时要注意的破坏性变化。

## 要点

### 被**移除**的特性

| 特性 | 替代 |
|------|------|
| `throw(TypeList)` 动态异常规范 | `noexcept`（C++11 已弃用，C++20 移除） |
| `std::auto_ptr` | `unique_ptr`（C++17 已移除） |
| `std::tr1` | 直接用 `std::` |
| `std::is_literal_type` | 无直接替代（概念过于宽泛） |
| `std::result_of` | `std::invoke_result` |
| `std::get_temporary_buffer` | `aligned_alloc` 等 |

### 被**弃用**的特性（C++20 仍可用，有警告）

| 特性 | 替代 | 原因 |
|------|------|------|
| `[=]` 隐式捕获 this | `[this]` 或 `[*this]` | 易错，歧义 |
| `volatile` 复合赋值 `v += 1` | `v = v + 1` 或 `atomic` | volatile 多线程语义不对 |
| `char8_t` 之前的 `u8''` 是 char | 用 `char8_t` | 类型区分 |
| `std::is_pod` | `is_trivially_copyable`/`is_standard_layout` | POD 概念过宽 |
| `std::is_literal_type` | 细化的 traits | 过宽 |
| C++ 头文件 `<ccomplex>` 等 | `<complex>` 等 | 冗余 |
| `std::char_traits<char8_t>` 特化 | 直接用 | 调整 |
| 隐式捕获 `*this` 的 `[=]` | 显式 `[*this]` | 同上 |

### `char8_t` 的破坏性变化

```cpp
// C++17：u8"" 是 const char[]
const char* s17 = u8"hello";   // C++17 OK

// C++20：u8"" 是 const char8_t[]
const char8_t* s20 = u8"hello";   // C++20 OK
const char* s = u8"hello";         // C++20 错误！char8_t* 不能转 char*
```

老代码 `const char* s = u8"..."` 在 C++20 编译失败，要改 `const char8_t*` 或 `reinterpret_cast`。

### `volatile` 部分弃用

C++20 弃用了 volatile 的以下用法：
- 复合赋值：`v += 1`（改为 `v = v + 1`）
- 自增自减：`v++`（改为 `v = v + 1`）
- `volatile` 返回值的隐式构造

**volatile 本身没被弃用**——只是部分易错用法。多线程共享变量仍该用 `atomic` 而非 volatile。

### `noexcept` 的完善

C++20 修复了 `noexcept` 作为类型系统一部分的一些边缘情况（C++17 引入但有问题）。

### `[[no_unique_address]]`（C++20 新增，非弃用）

```cpp
// 空成员不占空间（EBO 的泛化）
struct Empty {};
struct Foo {
    [[no_unique_address]] Empty e;   // 不占空间
    int x;
};
// sizeof(Foo) == 4（而非 8）
```

这不是弃用而是新增，但属于 C++20 的"小改进"——让空成员/分配器不增加类大小。

## HFT 关联

- **清理 `[=]` 隐式捕获 this**：HFT 策略类的 lambda 改用 `[this]`/`[*this]`，C++20 不容忍 `[=]` 的歧义。
- **`char8_t` 迁移**：用 `u8""` 的代码改 `const char8_t*`，或统一用 `const char*` + UTF-8 编码不依赖 `u8` 前缀。
- **`volatile` → `atomic`**：HFT 不该用 volatile 做线程通信，C++20 弃用部分用法是迁移信号——全改 `atomic`。
- **`[[no_unique_address]]` 减小对象**：策略类里的空分配器成员用 `[[no_unique_address]]` 不占空间，cache 友好。
- **`is_pod` → 细化 traits**：HFT 用 `is_trivially_copyable`（可 memcpy）+ `is_standard_layout`（C 兼容）替代过宽的 `is_pod`。
- **`throw()` → `noexcept`**：老代码的动态异常规范 C++20 移除，全改 `noexcept`。

## 自测题

1. C++20 移除了哪些特性？弃用了哪些？
2. `[=]` 隐式捕获 this 为什么被弃用？推荐怎么写？
3. `char8_t` 在 C++20 的破坏性变化是什么？老代码怎么迁移？
4. `[[no_unique_address]]` 的作用？HFT 如何利用？
5. `is_pod` 为什么被弃用？用什么替代？

## 代码自测

### Q1: C++20 弃用和移除
```cpp
// C++20 移除
// 1. auto_ptr（C++17 已移除）
// 2. std::iterator 模板基类（C++17 弃用，C++20 移除）
//    旧写法: class MyIter : public std::iterator<std::forward_iterator_tag, T>
//    新写法: 直接定义 5 个 typedef

// C++20 弃用
// 1. 逗号运算符在 [] 中（索引）
arr[1, 2];  // C++20 弃用（原意是 arr[(1,2)] = arr[2]，容易误以为多维）

// 2. POSIX 函数名
std::isalpha 等不再接受非 char 类型

// 3. char8_t 相关的旧 API
```
> 为什么要移除 std::iterator 基类？影响什么代码？

<details>
<summary>答案与复习指引</summary>

**移除 `std::iterator` 的原因**：
- 设计缺陷：基类提供的 typedef 让概念检查失效（派生类"免费"获得迭代器概念，但不一定真正满足）
- C++20 concepts 要求迭代器显式满足概念，不应靠继承获得

**受影响的代码**：
```cpp
// 旧写法（C++17 弃用，C++20 移除）
class MyIterator : public std::iterator<std::forward_iterator_tag, int> {
    // 自动获得 value_type/difference_type/pointer/reference/iterator_category
};

// 新写法（C++20）
class MyIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = int*;
    using reference = int&;
    // ... 运算符重载
};
```

**迁移**：把继承改为手动定义 5 个 typedef。简单但量大（STL 源码、自定义容器/迭代器）。

**逗号运算符弃用**：`arr[1, 2]` 原本合法（逗号运算符返回最后一个值 = arr[2]），但极易误读为多维数组。C++20 弃用，C++23 移除。`operator[]` 后续可能支持多参数。

**复习：** → [弃用和移除](./README.md)
</details>
