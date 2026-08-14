# 03 · Ownership and Lifetimes

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（生命周期 / 型变 / PhantomData / split_at_mut）

---

## 一句话

**理论门槛章** — 所有权与引用法则、别名与优化、生命周期/HRTB/无界 lifetime、型变三分类、Drop Check、PhantomData、unsafe 拆分借用。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 所有权与引用 | [01-ownership-basics.md](./01-ownership-basics.md) |
| 2 | 别名与优化 | [02-aliasing.md](./02-aliasing.md) |
| 3 | 生命周期 | [03-lifetimes.md](./03-lifetimes.md) |
| 4 | 型变 | [04-variance.md](./04-variance.md) |
| 5 | Drop Check | [05-drop-check.md](./05-drop-check.md) |
| 6 | PhantomData | [06-phantom-data.md](./06-phantom-data.md) |
| 7 | 借用分离 | [07-split-borrows.md](./07-split-borrows.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/ownership.rs](./src/ownership.rs) | 字段级可变借用拆分（safe） |
| [src/lifetimes.rs](./src/lifetimes.rs) | Elision、HRTB、无界 lifetime 警示 |
| [src/variance.rs](./src/variance.rs) | 协变 / 不变性直觉 |
| [src/phantom.rs](./src/phantom.rs) | `PhantomData` 标记逻辑所有权 |
| [src/split_borrows.rs](./src/split_borrows.rs) | `split_at_mut` 式 unsafe 拆分 |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/03_Lifetime_Variance
cargo run
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| 生命周期基础 | [RFR 08-lifetimes](../../02-RFR/Chapter-01-Foundations/08-lifetimes.md) |
| HRTB | [RFR 08.2 HRTB](../../02-RFR/Chapter-02-Types/08-2-hrtb.md) |
| split_at_mut | [Book 19.1 demo](../../00-Book/19-advanced-features/19.1-unsafe-rust-demo/) |
| 上一章 | [02_Data_Layout](../02_Data_Layout/README.md) |
| 下一章 | [04_Type_Cast](../04_Type_Cast/README.md) · 类型转换 |

---

## 逻辑脉络

引用法则 → 生命周期机制 → 型变 → Drop Check / PhantomData → unsafe 拆分借用 → 进入 Type Conversions。

---

## 速记

## 三句背诵

1. **引用不能比 referent 活得久；`&mut` 不能有别名。**
2. **`&mut T` 对 T 不变；函数参数是逆变主来源。**
3. **不相交借用须 unsafe 证明 → `split_at_mut` 模式。**

## 自测

- [ ] 能解释协变/逆变/不变各一例
- [ ] 能说明何时需要 `PhantomData`
- [ ] 能对照 [split_borrows.rs](./src/split_borrows.rs) 说明为何需要 raw ptr

