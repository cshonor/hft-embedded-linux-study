# 第 34 章 常用 C++17 编译设置

**Common C++17 Settings**

## 本章讲什么

实际项目里启用 C++17 需要的编译选项、版本宏、以及各编译器的特性支持矩阵。帮你确认"能用哪些 C++17 特性"。

## 要点

### 启用 C++17

| 编译器 | 选项 |
|--------|------|
| GCC | `-std=c++17`（或 `-std=gnu++17` 保留 GNU 扩展） |
| Clang | `-std=c++17` |
| MSVC | `/std:c++17` |

### 版本检测宏

```cpp
// __cplusplus 在 MSVC 不准（除非 /Zc:__cplusplus）
#if __cplusplus >= 201703L
    // C++17 或更高
#endif

// 更可靠：特征宏（<version> 头，C++20）/ <ciso646>
#ifdef __cpp_structured_bindings
    auto [x, y] = p;
#endif

// MSVC 特殊：_MSVC_LANG
#if defined(_MSVC_LANG) ? _MSVC_LANG >= 201703L : __cplusplus >= 201703L
```

### C++17 特性特征宏

```cpp
__cpp_structured_bindings      // 结构化绑定
__cpp_if_constexpr             // if constexpr
__cpp_fold_expressions         // 折叠表达式
__cpp_template_auto            // auto 模板参数
__cpp_inline_variables         // inline 变量
__cpp_variant                  // std::variant
__cpp_optional                 // std::optional
__cpp_string_view              // std::string_view
__cpp_lib_filesystem           // <filesystem>
__cpp_lib_parallel_algorithm   // 并行 STL
__cpp_lib_to_chars             // to_chars/from_chars
__cpp_lib_any                  // std::any
__cpp_lib_byte                 // std::byte
__cpp_lib_pmr                  // std::pmr
```

### 推荐的常用选项组合

```bash
# GCC/Clang 生产
-std=c++17 -O2 -Wall -Wextra -Wpedantic
# 严格：禁用扩展，纯标准
-std=c++17 -pedantic-errors

# MSVC
/std:c++17 /W4 /permissive-    # permissive- 严格标准模式
```

`/permissive-` 对 MSVC 很重要——它让 MSVC 更符合标准（默认 MSVC 有一些非标准行为）。

### 关键编译选项

| 选项 | 作用 |
|------|------|
| `-O2` / `-O3` | 优化级别（HFT 通常 `-O2` 或 `-O3` + LTO） |
| `-march=native` | 用本机 CPU 指令（AVX 等） |
| `-flto` | 链接期优化（跨文件内联） |
| `-DNDEBUG` | 关闭 assert（生产） |
| `-fno-exceptions` | 禁异常（HFT 热路径可选） |
| `-fno-rtti` | 禁 RTTI（减小二进制） |
| `-pthread` | 启用线程（Linux GCC/Clang） |

### 并行 STL 的后端

```bash
# GCC 9+：需要 TBB 后端
-std=c++17 -ltbb

# MSVC：内置 PPL 后端，无需额外库

# Clang：通常用 libstdc++ + TBB，或 libc++ + TBB
```

### CMake 配置

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   # 纯标准，不用 gnu++17

# 或 target 级
target_compile_features(mylib PUBLIC cxx_std_17)
```

## HFT 关联

- **`-O2`/`-O3` + LTO**：HFT 用 `-O2` 起步，关键路径测 `-O3`（更激进内联/向量化）+ `-flto`（跨文件内联）。
- **`-march=native`**：用本机 AVX/AVX2/AVX-512 指令，SIMD 信号计算必需。但部署时注意目标 CPU 是否支持。
- **`-fno-exceptions`/`-fno-rtti`**：HFT 热路径禁异常和 RTTI，减小二进制、消除 EH 表开销。但要确保库支持（STL 仍可用，只是 throw 变 terminate）。
- **`-DNDEBUG`**：生产关闭 assert，但 HFT 关键不变式检查建议保留（用自定义断言）。
- **特征宏做降级**：`#ifdef __cpp_lib_to_chars` 检测，无则降级用 `strtol`。
- **`/permissive-`（MSVC）**：跨平台项目必开，确保 MSVC 也符合标准。

## 自测题

1. 如何检测编译器是否支持 C++17？`__cplusplus` 的值是什么？
2. MSVC 检测 C++17 为什么用 `_MSVC_LANG`？
3. `/permissive-` 对 MSVC 有什么作用？为什么重要？
4. HFT 常用的编译选项组合是什么？`-fno-exceptions` 有什么影响？
5. CMake 如何设置 C++17 标准？`CMAKE_CXX_EXTENSIONS OFF` 的作用？

## 代码自测

### Q1: 编译选项
```bash
# C++17 编译
g++ -std=c++17 -O2 file.cpp
# 或 c++1z（C++17 确定前的过渡名）

# 常用搭配
g++ -std=c++17 -O2 -Wall -Wextra -pedantic file.cpp
```
> C++17 编译需要什么版本的编译器？常用 C++17 特性需要哪些 flag？

<details>
<summary>答案与复习指引</summary>

**编译器版本要求**：
| 编译器 | 最低版本 | 完整 C++17 |
|--------|---------|-----------|
| GCC | 7.1 | 8+ |
| Clang | 5.0 | 6+ |
| MSVC | 19.14 (VS 2017 15.7) | VS 2019 |

**特殊 flag**：
- `filesystem`：GCC 8 前需要 `-lstdc++fs`，GCC 9+ 自动链接
- `parallel STL`：需要 `-ltbb`（Threading Building Blocks）
- `pmr`：无需额外 flag

**HFT 推荐**：`-std=c++17 -O2 -Wall -Wextra -pedantic -flto -march=native`。`-flto` 启用链接时优化（跨文件内联），`-march=native` 启用目标 CPU 的 SIMD 指令。

**复习：** → [编译设置](./README.md)
</details>
