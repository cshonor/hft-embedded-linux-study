# 第 35 章 弃用与移除特性

**Deprecated and Removed Features**

## 本章讲什么

C++17 弃用了一些老旧特性、移除了 C++11/14 已弃用的部分。迁移到 C++17 时要注意这些变化，避免踩坑。

## 要点

### 被**移除**的特性（C++17 起不再存在）

| 特性 | 替代 |
|------|------|
| `std::auto_ptr` | `std::unique_ptr`（C++11 移除） |
| `std::register` 关键字 | 无（已无语义） |
| `tr1` 命名空间 | 直接用 `std::`（C++11 起） |
| `std::bind1st`/`std::bind2nd` | `std::bind` / lambda |
| `std::unexpected`/`set_unexpected` | 无（C++11 移除异常规范） |

### 被**弃用**的特性（C++17 仍可用，但有警告）

| 特性 | 替代 | 说明 |
|------|------|------|
| `std::result_of` | `std::invoke_result` | result_of 有已知问题 |
| `std::is_literal_type` | `is_trivially_copyable` 等 | 过于宽泛 |
| `std::raw_storage_iterator` | `uninitialized_copy` 等 | 不安全 |
| `std::get_temporary_buffer` | `aligned_alloc` 等 | 难用 |
| 三字符组（trigraphs `??=`） | 无 | 移除，C++17 不再支持 |
| `register` 关键字 | 无 | C++17 完全移除（保留但无语义→移除） |

### 库弃用

```cpp
// C++17 弃用的库组件
std::iterator<>           // 弃用，直接用裸 typedef
std::is_literal_type      // 弃用
std::result_of            // 弃用，用 invoke_result
std::pointer_to_binary_function  // 弃用，用 lambda/function
```

### `[[deprecated]]` 属性（C++14 引入，C++17 常用）

```cpp
[[deprecated("use new_func instead")]]
void old_func();

[[deprecated]] class OldClass {};
```

### C++17 对 C 标准库的调整

- `<ccomplex>`/`<cstdalign>`/`<cstdbool>`/`<ctgmath>` 弃用（C++ 的这些已和 C 脱钩）。
- C 的 `_Complex`/`_Bool`/`_Alignas` 等不用，用 C++ 的 `complex`/`bool`/`alignas`。

### 移除的影响

- 老代码用 `auto_ptr`/`register`/trigraph 的，C++17 编译失败。
- 弃用的特性编译时加 `-Wall` 会有警告，但仍可用。建议迁移。

## HFT 关联

- **清理 `auto_ptr`**：老 HFT 代码若有 `auto_ptr`，迁移到 `unique_ptr`——所有权语义清晰、零开销。
- **`result_of` → `invoke_result`**：泛型代码的返回类型推导换名字。
- **`[[deprecated]]` 标记旧 API**：策略接口升级时标记旧函数，编译期警告提醒迁移。
- **禁用 trigraph**：C++17 移除 trigraph，老代码若有 `??=` 等需改写（罕见）。
- **`register` 移除**：老 C 代码移植到 C++ 时 `register` 关键字要删（已无语义，C++17 移除）。
- **代码库现代化**：借迁移 C++17 的机会清理弃用特性，用 `[[deprecated]]` 渐进式标记。

## 自测题

1. C++17 移除了哪些特性？（auto_ptr、register、trigraph、bind1st/bind2nd）
2. 弃用和移除的区别？弃用的特性还能用吗？
3. `std::result_of` 为什么被弃用？用什么替代？
4. `[[deprecated]]` 属性如何使用？能带消息吗？
5. HFT 迁移 C++17 时要清理哪些老旧特性？

## 代码自测

### Q1: 被移除的特性
```cpp
// C++17 移除/弃用的特性
// 1. auto_ptr → 用 unique_ptr
// 2. register 关键字 → 移除
// 3. trigraphs (??= 等) → 移除
// 4. bool++ → 移除
// 5. throw() 动态异常规范 → 用 noexcept

// C++17 弃用（仍可用但警告）
[[deprecated("use new_func instead")]]
void old_func();

// 三段式 for 循环中的迭代器检查
// std::iterator 模板基类 → 弃用（C++17），移除（C++20）
```
> C++17 移除了哪些 C++11/14 已弃用的特性？为什么要移除？

<details>
<summary>答案与复习指引</summary>

**移除的特性**：
1. `auto_ptr`（被 `unique_ptr` 替代，拷贝语义有隐患）
2. `register` 关键字（编译器早已忽略，提示存储位置）
3. trigraphs（`??=` → `#` 等，为非 ASCII 键盘设计，已过时）
4. `bool` 的 `++` 操作（语义不明确）
5. `throw(type_list)` 动态异常规范（运行时开销大，被 `noexcept` 替代）
6. `std::iterator` 模板基类（设计有缺陷，直接定义 typedef 更好）

**为什么移除**：C++ 标准每版会清理已弃用的特性，减少语言复杂度。弃用 → 移除的周期通常 2 个标准版本（~6 年），给迁移时间。

**HFT 迁移**：检查代码中是否有 `auto_ptr`/`register`/`throw()`，替换为现代等价物。`-Wdeprecated-declarations` 警告可帮助发现。

**复习：** → [移除的特性](./README.md)
</details>
