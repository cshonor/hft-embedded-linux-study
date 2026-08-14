# 01 · Meet Safe and Unsafe

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md) · 开篇

## 状态

- [x] 已读（笔记整理）
- [x] 深度总结（Safe/Unsafe · HFT 适配）
- [x] 示例 crate（五种 unsafe 能力 + privacy 封装）

---

## 一句话

**心智模型章** — Safe / Unsafe 双重世界、`unsafe` 契约、五种额外能力、信任不对称、安全性非局部性；为全书 unsafe 编程定调。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 一 | 为何分 Safe/Unsafe | [01-why-safe-unsafe.md](./01-why-safe-unsafe.md) |
| 二 | unsafe 两种作用 | [02-unsafe-contract.md](./02-unsafe-contract.md) |
| 三 | 五种高危能力 | [03-five-powers.md](./03-five-powers.md) |
| 四 | 信任与非局部性 | [04-trust-and-nonlocality.md](./04-trust-and-nonlocality.md) |
| 五 | 易错疑问 | [05-faq.md](./05-faq.md) |
| 六 | HFT 实操规范 | [06-hft-practice.md](./06-hft-practice.md) |
| 七 | 裸指针完整解读 | [07-raw-pointers.md](./07-raw-pointers.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/five_powers.rs](./src/five_powers.rs) | 五种 unsafe 能力 |
| [src/raw_pointers.rs](./src/raw_pointers.rs) | `*const` / `*mut` 创建 vs 解引用 |
| [src/privacy.rs](./src/privacy.rs) | 模块边界封装 invariant |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/01_Safe_Unsafe
cargo run
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| unsafe 入门 | [The Book 19.1](../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| 深入 | [RFR Ch09 Unsafe](../../02-RFR/Chapter-09-Unsafe-Code/README.md) |
| Miri 验证 | [ER Item 16 demo](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/) |
| 下一章 | [02_Data_Layout](../02_Data_Layout/README.md) · 数据布局 |

---

## 逻辑脉络

设计出发点 → 契约机制 → 五种能力对照源码 → 信任与非局部性 → FAQ → HFT 规范 → 进入 Data Layout。

---

## 速记

## 三句背诵

1. **Safe = 编译器兜底；Unsafe = 程序员兜底，语法相同、多 5 种能力。**
2. **`unsafe fn/trait` 立契约；`unsafe {}` / `unsafe impl` 兑承诺。**
3. **Safe 无条件信 Unsafe；Unsafe 不能信 Safe 用户；安全是非局部的 → privacy 封装。**

## 5 种能力（仅 unsafe 块内）

1. 解引用 raw pointer（**创建**指针本身 Safe；解引用/偏移/转引用须 unsafe）
2. 调用 unsafe fn（含 FFI / intrinsic）
3. `unsafe impl` unsafe trait
4. 读/写 `static mut`
5. 读/写 `union` 字段

## 自测

- [ ] 能解释 `Vec::push` 为何对外是 Safe fn
- [ ] 能说明 `unsafe fn` 空函数体为何调用仍需 `unsafe {}`
- [ ] 能举例说明「Safe 改一行 → 全库 unsound」
- [ ] 能列出 HFT 场景下 unsafe 的典型用途（FFI / DPDK / 自定义分配器）
- [ ] 能区分「创建裸指针 Safe」与「解引用须 unsafe」
- [ ] 能对照 [raw_pointers.rs](./src/raw_pointers.rs) 说明 `*const` vs `*mut`

## 术语表（本章）

| 术语 | 含义 |
|------|------|
| UB | 未定义行为；编译器可任意假设未发生 |
| unsound | 库在 Safe 接口下仍可能 UB |
| invariant | unsafe 层维护的全局不变量（如 Vec len/cap/ptr） |
| 裸指针 | 无生命周期/借用的地址；解引用须 unsafe |

