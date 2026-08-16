# 四 · 信任不对称与非局部性

← [本章目录](./README.md) · 上一节：[03-five-powers.md](./03-five-powers.md) · 下一节：[05-faq.md](./05-faq.md)

---

写底层库必须掌握的两条思想；**非局部性**是本章最难、最核心的知识点。

## 1. 信任不对称原则

Safe 代码和 Unsafe 代码互相信任的规则不对等：

### Safe → Unsafe：无条件信任

Safe Rust 没有任何手段校验 unsafe 内部逻辑是否合规。只要调用不安全函数，编译器默认 unsafe 代码编写者遵守了所有契约，不会额外检查。一旦 unsafe 内部写错，整个程序出现 UB。

### Unsafe → Safe：不能信任上层 Safe 代码

unsafe 底层逻辑不能假设外部 Safe 业务代码无逻辑 bug。

**示例**：容器内部大量 unsafe 内存操作，但用户传入错误排序逻辑的 `Ord` 实现，最多触发 panic，**不会产生内存破坏**。底层 unsafe 代码必须做好隔离，隔离上层业务错误。

## 2. 安全性具备非局部性

### 定义

一段完全合法、无任何报错的 Safe 上层代码，修改后可能破坏底层 unsafe 维护的全局不变量，导致整个库变成 **unsound**（存在隐性 UB 漏洞）。

### 根源

unsafe 的正确性依赖跨代码全局不变量，不局限于 unsafe 代码块内部。

**举例**：`Vec` 维护 `len`、`capacity`、内部裸指针三者匹配的不变量。如果 `Vec` 对外暴露裸指针给 Safe 代码，外部 Safe 代码篡改指针地址，会破坏 `Vec` 内部平衡，后续 push/pop 直接 UB。

### 标准工程解决方案

利用 Rust **模块私有性（privacy）** 封装所有 unsafe 内部状态：

- 裸指针、`static mut`、`union` 等危险结构全部设为私有字段；
- 对外只提供封装好的 Safe API，隔绝外部 Safe 代码篡改内部不变量；
- 所有不安全操作收敛在模块内部，仅模块作者负责维护不变量。

→ 源码对照：[src/privacy.rs](./src/privacy.rs)
