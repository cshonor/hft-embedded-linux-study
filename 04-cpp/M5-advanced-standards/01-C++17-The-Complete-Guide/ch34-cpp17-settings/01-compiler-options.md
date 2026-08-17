# 编译器选项

## 启用 C++17

| 编译器 | 选项 | 最低版本 |
|--------|------|---------|
| GCC | `-std=c++17` | GCC 7.1 |
| Clang | `-std=c++17` | Clang 5.0 |
| MSVC | `/std:c++17` | VS 2017 15.7 |

```bash
# GCC/Clang
g++ -std=c++17 -O2 file.cpp

# 纯标准（禁用 GNU 扩展）
g++ -std=c++17 -pedantic-errors file.cpp

# 保留 GNU 扩展
g++ -std=gnu++17 file.cpp

# MSVC
cl /std:c++17 /permissive- file.cpp
```

## HFT 推荐选项

```bash
# 生产编译选项
-std=c++17
-O2                    # 或 -O3（测过再选）
-flto                  # 链接期优化（跨文件内联）
-march=native          # 本机 CPU 指令（AVX/SSE）
-DNDEBUG              # 关闭 assert
-Wall -Wextra -Wpedantic  # 警告
-pthread              # 线程支持

# 可选（热路径激进优化）
-fno-exceptions        # 禁异常（减小 EH 表开销）
-fno-rtti              # 禁 RTTI（减小二进制）
```

## 关键选项详解

### -O2 vs -O3

```bash
# -O2：安全优化（内联、循环优化、死代码消除）
# -O3：更激进（循环展开、向量化、更激进内联）
# HFT：默认 -O2，关键路径测 -O3 是否更好

# 注意：-O3 可能导致代码变大（指令缓存压力）
# 需要实测决定
```

### -march=native

```bash
# 用本机 CPU 的所有指令集
-march=native          # 自动检测

# 或指定具体架构
-march=haswell         # AVX2
-march=skylake-avx512  # AVX-512

# 注意：部署时目标 CPU 必须支持这些指令
# 否则 SIGILL（非法指令）崩溃
```

### -fno-exceptions / -fno-rtti

```bash
# 禁异常：
# - STL 仍可用，但 throw 变成 std::terminate
# - 消除异常表开销（二进制更小）
# - 减少函数 prologue 开销

# 禁 RTTI：
# - dynamic_cast 和 typeid 不可用
# - 消除 typeinfo 开销
# - 虚函数仍可用
```

## CMake 配置

```cmake
# 全局设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # 纯标准，不用 gnu++17

# target 级设置
target_compile_features(mylib PUBLIC cxx_std_17)

# HFT 选项
target_compile_options(mylib PRIVATE
    -O2 -flto -march=native
    -Wall -Wextra -Wpedantic
)
```

## 并行 STL 后端

```bash
# GCC 9+：需要 TBB
-ltbb

# GCC 8：需要 stdc++fs（filesystem）
-lstdc++fs

# MSVC：内置 PPL，无需额外库
```

## 自测题

1. GCC/Clang/MSVC 各自启用 C++17 的选项是什么？
2. `-std=c++17` 和 `-std=gnu++17` 的区别？`CMAKE_CXX_EXTENSIONS OFF` 做什么？
3. `-O2` 和 `-O3` 的区别？HFT 怎么选？
4. `-fno-exceptions` 有什么影响？STL 还能用吗？
5. `-march=native` 的风险是什么？
