# 1.3 · 自定义构建（CMake 变量）

← [02 五步构建](./02-build-five-steps.md)

---

LLVM 构建由 **CMake** 驱动 — 通过 `-D` 变量高度定制。

---

## 一、通用 CMake 变量

| 变量 | 说明 |
|------|------|
| **`CMAKE_BUILD_TYPE`** | `Debug` · `Release` · `RelWithDebInfo` · `MinSizeRel` |
| **`CMAKE_INSTALL_PREFIX`** | 安装路径（如 `/usr/local` 或 `$HOME/llvm-install`） |
| **`CMAKE_C_COMPILER`** | 指定 C 编译器 |
| **`CMAKE_CXX_COMPILER`** | 指定 C++ 编译器 |

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=$HOME/llvm-17 \
  -DCMAKE_CXX_COMPILER=clang++ \
  -B build -S llvm
```

---

## 二、LLVM 专属变量（初次安装常用）

| 变量 | 说明 |
|------|------|
| **`LLVM_TARGETS_TO_BUILD`** | 要构建的 **目标架构** — `X86;AArch64;ARM` 或 `all` |
| **`LLVM_EXPERIMENTAL_TARGETS_TO_BUILD`** | **实验性**后端 — 如书后 **M88k / M68k** 演示 |
| **`LLVM_ENABLE_PROJECTS`** | 随 LLVM 一起编的子项目 — `clang;lld;lldb` 等 |

```bash
-DLLVM_TARGETS_TO_BUILD="X86;AArch64" \
-DLLVM_ENABLE_PROJECTS="clang;lld"
```

---

## 三、LLVM 进阶变量

| 变量 | 说明 |
|------|------|
| **`LLVM_ENABLE_ASSERTIONS`** | 开发期开启断言 — Debug 排查 |
| **`LLVM_ENABLE_EH`** | 开启 C++ **异常处理**（LLVM 默认关，保持轻量） |
| **`LLVM_ENABLE_RTTI`** | 开启 **RTTI**（默认关） |
| **`LLVM_OPTIMIZED_TABLEGEN`** | Debug 构建时仍 **优化 TableGen** — 加速代码生成工具 |

> LLVM 默认关 EH/RTTI 是为了核心库轻量化；**开发自己的 LLVM 插件/后端**时按需打开。

---

## 四、示例：开发向 Debug 配置

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_OPTIMIZED_TABLEGEN=ON \
  -B build -S llvm
```

---

## 五、设计取舍

| 需求 | 建议 |
|------|------|
| 本机读 IR · 对照 rustc | 不编 LLVM · 用系统包 |
| 改 IR pass · 用 opt | 二进制 + dev 头文件 |
| 跟书 Part 04 加后端 | 源码 + 缩小 `TARGETS` + Debug/RelWithDebInfo |
| 磁盘紧张 | 只编 `X86` · Release · 不跑 `check-all` |

→ [00 本章定位 · 与仓库策略](./00-overview.md)
