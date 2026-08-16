# ChB · C compilers（C 编译器）

> **附录** · 策略：**⏭️ 参考**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本附录讲什么

gcc/clang/msvc 对各标准的支持表、编译选项速查。查表用。

## 一、编译器对 C 标准的支持

| 标准 | gcc 最低版本 | clang 最低版本 | MSVC |
|------|-------------|---------------|------|
| C89 | 全部 | 全部 | 全部 |
| C99 | 4.5（基本完整） | 3.0 | 2019 16.8（部分） |
| C11 | 4.7 | 3.1 | 2019 16.8（部分） |
| C17 | 8.0 | 7.0 | 2019 16.8 |
| C23 | 14.0（大部分） | 18.0（大部分） | 暂不支持 |

## 二、HFT 常用编译选项

| 选项 | 效果 |
|------|------|
| `-std=c11` / `-std=gnu11` | C11 标准（gnu = 加 GNU 扩展） |
| `-std=c2x` / `-std=gnu2x` | C23 标准（过渡名，正式版用 `-std=c23`） |
| `-Wall -Wextra` | 常用警告 |
| `-Wpedantic` | 严格标准合规（拒绝 GNU 扩展） |
| `-Wvla` | 警告 VLA 使用 |
| `-Wconversion` | 警告隐式类型转换 |
| `-O2` / `-O3` | 优化级别 |
| `-g3` | 调试信息 |
| `-fno-strict-aliasing` | 禁用严格别名优化（内核用） |
| `-march=native` | 针对当前 CPU 优化（AVX2 等） |
| `-mcmodel=large` | 大内存模型（HFT hugepage 可能需要） |

## 三、DPDK 编译建议

```bash
# DPDK meson 构建常用选项
meson setup build -Dc_args='-std=c11 -O3 -march=native'
ninja -C build
```

## 自测题

<details><summary>1. <code>-std=c11</code> 和 <code>-std=gnu11</code> 有什么区别？</summary>

`-std=c11` 使用纯 C11 标准，不包含 GNU 扩展。`-std=gnu11` 在 C11 基础上允许 GNU 扩展
（`__attribute__`、`typeof`、语句表达式等）。内核和 DPDK 都用 `gnu` 变体，因为依赖 GNU 扩展。
用 `-Wpedantic` 时 `gnu` 变体会警告扩展用法。
</details>
