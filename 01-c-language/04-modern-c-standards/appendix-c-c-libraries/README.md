# ChC · C libraries（C 库）

> **附录** · 策略：**⏭️ 参考**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本附录讲什么

glibc/musl 等实现差异。HFT 关注 musl（更小更快）与 glibc 差异。

## 一、主流 C 库实现

| 库 | 特点 | HFT 相关度 |
|----|------|-----------|
| **glibc** | Linux 默认，功能全，体积大 | 默认选择 |
| **musl** | 轻量、快速、静态链接友好 | HFT 容器/静态链接 |
| **bionic** | Android | 不用 |
| **newlib** | 嵌入式 | 不用 |

### glibc vs musl

| 方面 | glibc | musl |
|------|-------|------|
| 体积 | 大（动态库 ~2MB） | 小（~400KB） |
| malloc | ptmalloc（多 arena） | 简单实现 |
| 线程局部存储 | 性能好 | 性能好 |
| 静态链接 | ⚠️ 不推荐（某些函数有问题） | ✅ 设计支持静态链接 |
| 兼容性 | 最广 | 大部分 POSIX 应用 |
| HFT 选择 | 默认 | 容器化/最小镜像 |

## 二、HFT 相关差异

### malloc 实现差异

| 库 | malloc 策略 | HFT 影响 |
|----|------------|----------|
| glibc | ptmalloc（多 arena，小对象 fastbin） | 多线程性能好，但延迟波动 |
| musl | 简单 malloc（mmap 大块 + 切分） | 更可预测但可能较慢 |

> **HFT 立场**：热路径不用任何 C 库的 malloc——预分配内存池。C 库的 malloc 只在初始化阶段用。

### `__STDC_VERSION__` 和特性宏

```c
/* 检测 C 库特性 */
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
  /* glibc: __GLIBC__ == 2, __GLIBC_MINOR__ == 34 等 */
#endif

#if defined(__MUSL__)
  /* musl */
#endif
```

## 自测题

<details><summary>1. HFT 为什么不在意 glibc vs musl 的 malloc 差异？</summary>

因为 HFT 热路径完全不用 C 库的 malloc——启动时从 hugepage 预分配内存池，运行时从池中 O(1)
分配/释放。C 库的 malloc 只在初始化阶段使用，其性能差异不影响热路径。选择 glibc 还是 musl
主要看部署需求（容器镜像大小、静态链接等）。
</details>
