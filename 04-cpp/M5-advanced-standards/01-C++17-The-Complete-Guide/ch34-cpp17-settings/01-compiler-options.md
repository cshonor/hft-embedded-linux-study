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

<details>
<summary>参考答案</summary>

1. - **GCC**：`-std=c++17`（另有 `-std=c++1z` 是早期的同义写法）。
   - **Clang**：`-std=c++17`。
   - **MSVC**：`/std:c++17`（VS2017 15.7 起支持；`/std:c++latest` 表示跟随最新标准，不固定版本）。
CMake 里通常写 `set(CMAKE_CXX_STANDARD 17)` + `set(CMAKE_CXX_STANDARD_REQUIRED ON)`，由 CMake 生成对应编译器的开关。
2. `-std=c++17` 严格按 ISO C++17，禁用编译器扩展；`-std=gnu++17` 在 C++17 基础上**启用 GNU 扩展**（如某些内建函数、匿名结构体、变长数组等），GCC/Clang 的默认通常就是 `gnu++*`。
`CMAKE_CXX_EXTENSIONS OFF` 告诉 CMake **不要**加 `-std=gnu++17`，而是用 `-std=c++17`——即关闭编译器扩展。常与 `CMAKE_CXX_STANDARD 17` 和 `CMAKE_CXX_STANDARD_REQUIRED ON` 一起用，保证可移植、不被悄悄绑到 GNU 方言上。
3. `-O2` 打开绝大多数优化（内联、常量传播、循环优化等），但会避开那些**显著增大代码体积**或收益不确定的变换。
`-O3` 在 `-O2` 之上更激进：更大的内联阈值、更多循环展开、更积极的向量化、可能启用 `-ftree-loop-distribute-patterns` 等，代价是代码体积变大、编译更慢、icache/分支预测压力增加，偶尔还会因重排/向量化让浮点结果与 `-O2` 有细微差异。
HFT 怎么选：没有普适答案，以**实测的延迟分布（尤其是尾延迟）**为准。常见做法是从 `-O2` 起步，试 `-O3` + `-flto`，用 perf/llvm-mca 看热点；并避免 `-ffast-math` 一类会改变浮点语义的选项，除非能接受结果不可复现。
4. 影响：去掉异常展开表与 throw 路径，二进制更小、代码路径更少（延迟更可预测），是低延迟/嵌入式常见配置。
STL 仍可使用：容器、算法、智能指针等都能编译和运行；但**异常语义失效**——抛出时不会展开，而是直接 `std::terminate`/abort；`operator new` 失败也不会抛 `std::bad_alloc`。部分依赖异常的库组件行为会改变或不可用（不同标准库实现程度不同）。
注意：`-fno-exceptions` 必须在**所有编译单元和依赖库**上保持一致，混用会造成 ABI/ODR 不一致；同时通常配套 `-fno-rtti` 以进一步减小体积（代价是不能再用 `dynamic_cast` / `typeid`）。
5. `-march=native` 让编译器按**当前编译机器**的 CPU 特性生成指令（AVX2、AVX-512、特定 BMI 指令等）。
风险：
   - 二进制**不可移植**——部署到缺少对应指令集的机器上会 **SIGILL 崩溃**；
   - 编译机与生产机 CPU 不一致时，构建产物与预期不符；
   - 容器/编译农场场景尤其危险（编译节点和跑的节点不同）；
   - 破坏可复现构建，且不同核之间可能有细微差异。
建议：要么确保编译环境与运行环境一致，要么**显式指定目标架构**（如 `-march=skylake-avx512`、`-mavx2`），把指令集选择写进构建配置而不是交给 `native` 猜。

</details>
