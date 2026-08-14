# 04 · Type Conversions

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（coercion / dot / cast / transmute）

---

## 一句话

**转换阶梯章** — 隐式 coercion → 点运算符 Deref/unsizing → `as` 显式 cast（非传递性）→ `transmute` 终极警告。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 隐式转换 | [01-coercions.md](./01-coercions.md) |
| 2 | 点运算符 | [02-dot-operator.md](./02-dot-operator.md) |
| 3 | 显式转换 `as` | [03-casts.md](./03-casts.md) |
| 4 | transmute | [04-transmutes.md](./04-transmutes.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/coercions.rs](./src/coercions.rs) | 隐式弱化、`&dyn Trait` |
| [src/dot_operator.rs](./src/dot_operator.rs) | Auto-ref、Deref、unsizing 找方法 |
| [src/casts.rs](./src/casts.rs) | 数字 cast、指针 cast、非传递性 |
| [src/transmute.rs](./src/transmute.rs) | 同尺寸 reinterpret（含 UB 警示） |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/04_Type_Cast
cargo run
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| Deref coercion | [ER Item 05 demo](../../01-ER/Chapter-01-Types/Item-05-type-conversions/demo/) |
| 布局与 transmute | [RFR 08-casting](../../02-RFR/Chapter-09-Unsafe-Code/08-casting.md) |
| 上一章 | [03_Lifetime_Variance](../03_Lifetime_Variance/README.md) |
| 下一章 | [05_Uninit_Mem](../05_Uninit_Mem/README.md) · 未初始化内存 |

---

## 逻辑脉络

无害 coercion → 点运算符查找链 → `as` 显式边界 → transmute 禁区 → 进入 Uninitialized Memory。

---

## 速记

## 三句背诵

1. **Coercion 自动弱化类型；Trait 匹配不自动转，receiver 除外。**
2. **点运算符：按值 → auto-ref → Deref 链 → unsizing 找方法。**
3. **`as` 比 coercion 宽但非传递；transmute 只验 size，&T→&mut T 永远 UB。**

## 自测

- [ ] 能举例 `&String` → `&str`、`&T` → `&dyn Trait` 的 coercion
- [ ] 能复述 `value.foo()` 的四步查找顺序
- [ ] 能解释 `e as U1 as U2` 合法为何 `e as U2` 可能非法
- [ ] 能说明 transmute 与 `as` 的本质区别（size vs 语义）
- [ ] 能对照 [coercions.rs](./src/coercions.rs) / [casts.rs](./src/casts.rs) 指出各转换类型

## 术语表

| 术语 | 含义 |
|------|------|
| coercion | 编译器自动、通常无害的类型弱化 |
| unsizing | 定长数组/Trait 对象等「变胖」的 coercion |
| cast (`as`) | 显式转换，范围大于 coercion，可能截断 |
| transmute | 同尺寸比特 reinterpret，几乎无防护 |
| 非传递性 | 分步 cast 合法 ≠ 一步 cast 合法 |

