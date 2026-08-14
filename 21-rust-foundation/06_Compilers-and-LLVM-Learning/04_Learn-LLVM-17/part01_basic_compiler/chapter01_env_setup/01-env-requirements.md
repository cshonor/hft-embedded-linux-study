# 1.1 · 准备和配置环境

← [00 本章定位](./00-overview.md) · 下一节 [02 五步构建](./02-build-five-steps.md)

---

## 为何不只装二进制？

许多 Linux 发行版（如 Ubuntu `llvm-dev`）提供 LLVM **二进制包** — 日常够用。

**从源码编译**的好处：

- 确保含**全部开发文件**（头文件、TableGen、后端源码树）
- 熟悉 **CMake 可定制性** — 后续 **添加新后端**（Part 04）必需

---

## 硬件要求

| 场景 | 磁盘 | 其他 |
|------|------|------|
| **Debug + 符号** | 最多约 **30 GB** | 多核 CPU（如 2.5 GHz 四核）+ **SSD** 显著缩短编译时间 |
| **Release · 单平台** | 最少约 **2 GB** | |

---

## 编译器要求（LLVM 17 · C++17）

| 编译器 | 最低版本（书中） |
|--------|------------------|
| **GCC** | 7.1.0+ |
| **Clang** | 5.0+ |
| **Apple Clang** | 10.0+ |
| **Visual Studio** | 2019 16.7+ |

---

## 必备工具

| 工具 | 版本要求 | 用途 |
|------|----------|------|
| **Git** | ≥ 2.39.1 | 克隆 llvm-project |
| **CMake** | ≥ 3.20.0 | 生成构建文件 |
| **Ninja** | ≥ 1.11.1（推荐） | 快速构建 |
| **Python** | ≥ 3.8 | 生成脚本 · 运行测试 |
| **zlib** | ≥ 1.2.3.4 | LLVM 依赖 |

### 平台安装（书中示例 · 按需查阅原书）

| 平台 | 典型包管理 |
|------|------------|
| **Ubuntu/Debian** | `apt install git cmake ninja-build python3 zlib1g-dev` + 编译器 |
| **Fedora/RHEL** | `dnf install git cmake ninja python3 zlib-devel` |
| **FreeBSD** | ports / pkg |
| **macOS** | Xcode CLT 或 Homebrew |
| **Windows** | Visual Studio + Git · CMake · Python；克隆时建议 `--config core.autocrlf=false` |

> 具体命令以原书 / 当前 LLVM 文档为准 — 本章 **浏览** 记版本门槛即可。

---

## 本仓库最小验收（IR 路线）

不必完成全书源码编译，满足即可继续 Part 02：

```bash
clang --version    # 或 llvm-config --version
# 能 cargo rustc -- --emit=llvm-ir 并打开 .ll
```

→ [../../README.md](../../README.md) · `llvm_insight_lab`
