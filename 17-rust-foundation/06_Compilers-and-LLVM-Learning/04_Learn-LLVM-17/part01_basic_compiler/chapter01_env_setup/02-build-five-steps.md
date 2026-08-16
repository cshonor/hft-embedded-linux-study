# 1.2 · 从源码克隆并构建（五步）

← [01 环境要求](./01-env-requirements.md) · 下一节 [03 自定义 CMake](./03-custom-cmake-vars.md)

---

## 总流程（各平台一致）

```text
配置 Git → 克隆 llvm-project → 独立 build/ → CMake 配置 → 编译 / 测试 / 安装
```

---

## ① 配置 Git

```bash
git config --global user.email "you@example.com"
git config --global user.name "Your Name"
```

---

## ② 克隆代码库

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
```

**Windows** — 避免 CRLF 破坏构建脚本：

```bash
git clone --config core.autocrlf=false https://github.com/llvm/llvm-project.git
```

**切发布分支**（与书对齐 LLVM 17）：

```bash
git checkout llvmorg-17.0.1   # 或当前需要的 tag
```

---

## ③ 创建构建目录（禁止内联构建）

LLVM **不支持**在源码树内直接 `make` — 必须**源码外**独立目录：

```bash
mkdir build && cd build
# 或在 llvm-project 同级：llvm-project/ 与 build/
```

---

## ④ CMake 生成构建文件

典型 **Release + Clang 子项目**：

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS=clang \
  -B build -S llvm
```

| 参数 | 含义 |
|------|------|
| `-G Ninja` | 使用 Ninja 生成器 |
| `-DCMAKE_BUILD_TYPE=Release` | 发布优化构建 |
| `-DLLVM_ENABLE_PROJECTS=clang` | 同时构建 **Clang** |
| `-B build -S llvm` | 构建目录 `build`，源码 `llvm/` 子目录 |

> 在 `llvm-project` 根执行时，`-S llvm` 指向 monorepo 内 `llvm` 子文件夹。

---

## ⑤ 编译 · 测试 · 安装

```bash
# 编译（限制并行，避免占满机器）
cmake --build build -j2

# 可选：运行测试（耗时长）
cmake --build build --target check-all

# 安装到默认或 CMAKE_INSTALL_PREFIX
cmake --install build
```

| 步骤 | 说明 |
|------|------|
| **`-j N`** | 限制并行 job — 如 `-j2` 防卡顿 |
| **`check-all`** | 全量测试 — 首次验证可选 |
| **`install`** | 将 `clang`/`opt`/`llc` 等装到 prefix |

---

## 与本仓库关系

| 目标 | 是否需要走完五步 |
|------|------------------|
| 读 **rustc IR** · `ir_samples/` | ❌ 通常不需要 |
| 玩 **opt/llc** 对照 O0/O3 | 二进制 LLVM 即可 |
| **Part 04 自定义后端** | ✅ 需要源码树 + 自定义 CMake |

→ 下一步：[03 自定义 CMake](./03-custom-cmake-vars.md)
