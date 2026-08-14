# 特性特征宏

## 版本检测

```cpp
// 方法1：__cplusplus（MSVC 不准）
#if __cplusplus >= 201703L
    // C++17 或更高
#endif

// 方法2：_MSVC_LANG（MSVC 专用）
#if defined(_MSVC_LANG)
    #if _MSVC_LANG >= 201703L
        // MSVC C++17
    #endif
#else
    #if __cplusplus >= 201703L
        // GCC/Clang C++17
    #endif
#endif

// 方法3：统一宏
#if (defined(_MSVC_LANG) ? _MSVC_LANG : __cplusplus) >= 201703L
    // C++17
#endif
```

## 特性特征宏

```cpp
// 语言特性
__cpp_structured_bindings      // 结构化绑定
__cpp_if_constexpr             // if constexpr
__cpp_fold_expressions         // 折叠表达式
__cpp_template_auto            // auto 模板参数
__cpp_inline_variables         // inline 变量
__cpp_nontype_auto             // auto NTTP

// 库特性
__cpp_lib_variant              // std::variant
__cpp_lib_optional             // std::optional
__cpp_lib_any                  // std::any
__cpp_lib_byte                 // std::byte
__cpp_lib_string_view          // std::string_view
__cpp_lib_filesystem           // <filesystem>
__cpp_lib_parallel_algorithm   // 并行 STL
__cpp_lib_to_chars             // to_chars/from_chars
__cpp_lib_pmr                  // std::pmr
__cpp_lib_type_trait_variable_templates  // _v 变量模板
```

## 使用示例

```cpp
// 条件编译：有 to_chars 就用，没有就降级
#ifdef __cpp_lib_to_chars
    auto [ptr, ec] = std::to_chars(buf, buf+32, val);
#else
    snprintf(buf, sizeof(buf), "%d", val);
#endif

// 检测结构化绑定
#ifdef __cpp_structured_bindings
    auto [x, y] = get_pair();
#else
    auto p = get_pair();
    auto x = p.first;
    auto y = p.second;
#endif
```

## 检查编译器支持矩阵

```cpp
// 打印支持的特性
#include <version>  // C++20，C++17 用 <ciso646>

void print_features() {
#ifdef __cpp_structured_bindings
    std::cout << "structured bindings: yes\n";
#endif
#ifdef __cpp_lib_filesystem
    std::cout << "filesystem: yes\n";
#endif
#ifdef __cpp_lib_to_chars
    std::cout << "to_chars: yes\n";
#endif
#ifdef __cpp_lib_pmr
    std::cout << "pmr: yes\n";
#endif
}
```

## HFT 应用

```cpp
// 降级策略：编译期检测，选择最优实现
template <typename T>
std::string to_str_fast(T val) {
#ifdef __cpp_lib_to_chars
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf+32, val);
    return std::string(buf, ptr);
#else
    return std::to_string(val);  // 降级
#endif
}
```

## 自测题

1. `__cplusplus` 在 MSVC 上的问题是什么？怎么解决？
2. 特性特征宏的命名规则是什么？（语言 vs 库）
3. 如何用特征宏做条件编译降级？
4. `__cpp_lib_to_chars` 检测的是什么？
5. C++20 的 `<version>` 头有什么用？
