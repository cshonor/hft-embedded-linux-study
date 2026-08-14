# C++17 移除的特性

## 被移除的特性（不再存在）

| 特性 | C++ 版本 | 替代 |
|------|---------|------|
| `std::auto_ptr` | C++11 弃用→C++17 移除 | `std::unique_ptr` |
| `std::register` 关键字 | C++11 弃用→C++17 移除 | 无（编译器早已忽略） |
| 三字符组 trigraphs | C++17 移除 | 无 |
| `std::bind1st`/`bind2nd` | C++11 弃用→C++17 移除 | `std::bind`/lambda |
| `std::unexpected` | C++11 弃用→C++17 移除 | 无（异常规范移除） |
| `bool` 的 `++` 操作 | C++17 移除 | 无 |
| `throw(type_list)` | C++11 弃用→C++17 移除 | `noexcept` |
| `std::iterator` 基类 | C++17 弃用→C++20 移除 | 直接定义 typedef |

## auto_ptr 的问题

```cpp
// auto_ptr：拷贝语义是转移所有权（反直觉）
std::auto_ptr<int> a(new int(42));
std::auto_ptr<int> b = a;  // 拷贝？不，是转移！
// a 现在为空，b 持有指针

// 在容器中灾难性：
std::vector<std::auto_ptr<int>> v;
v.push_back(std::auto_ptr<int>(new int(1)));
v.push_back(std::auto_ptr<int>(new int(2)));
// 内部拷贝可能导致悬空指针

// unique_ptr：移动语义明确
std::unique_ptr<int> a = std::make_unique<int>(42);
// auto b = a;  // ❌ 编译错误（不可拷贝）
auto b = std::move(a);  // ✅ 显式移动
```

## trigraph 移除

```cpp
// 三字符组：为没有 # 等字符的键盘设计（古老 IBM 终端）
??=   →  #
??(   →  [
??)   →  ]

// C++17 移除：
// ??=include <iostream>  // C++17 前合法（= #include）
// C++17 编译错误
```

## throw() 动态异常规范

```cpp
// C++03：动态异常规范
void foo() throw(std::bad_alloc);  // 只允许抛 bad_alloc
void bar() throw();                 // 不抛异常

// C++11：noexcept 替代
void foo() noexcept;  // 不抛异常

// C++17：throw() 完全移除
// void foo() throw(std::bad_alloc);  // ❌ 编译错误
void foo() noexcept;  // ✅
```

## 迁移检查

```bash
# GCC/Clang 警告
-Wdeprecated-declarations  # 弃用警告
-Werror=deprecated-declarations  # 弃用变错误

# 搜索老代码
grep -r "auto_ptr" src/
grep -r "register " src/  # 注意：register 作为变量名不算
grep -r "throw(" src/
grep -r "??=" src/  # trigraph
```

## 自测题

1. C++17 移除了哪些特性？列出至少 5 个。
2. `auto_ptr` 的拷贝语义有什么问题？`unique_ptr` 如何改进？
3. trigraph 是什么？为什么移除？
4. `throw()` 和 `noexcept` 的区别？为什么 `throw()` 被移除？
5. 迁移 C++17 时如何检测代码中的老旧特性？
