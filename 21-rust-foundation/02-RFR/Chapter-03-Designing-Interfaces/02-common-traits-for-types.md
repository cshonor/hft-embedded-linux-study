# 1.2 Common Traits for Types（类型的通用 Trait）

> 所属：**Unsurprising** · [← 章索引](./README.md)

← [01 命名](./01-naming-practices.md) · 下一节 [03 人体工程学 impl](./03-ergonomic-trait-implementations.md)

用户期望公开类型在常见场景下「开箱即用」。

---

## 适用谁？——最佳实践，不是语法铁律

| 场景 | 要不要卡三类划分 |
|------|------------------|
| **对外公开库 / crate 的类型** | ✅ 建议遵守 — RFR / ER 给**库作者**的 Unsurprising 指南 |
| **业务代码、内部工具、一次性脚本** | ⚪ **不必死板** — 编译器不强制你 derive `Debug` |
| **自定义 Trait**（如量化 `Strategy`） | 📦 **另归「按需设计」** — 不属于 std 三类；给别人用时仍要接口清晰 |
| **自定义类型 impl 标准 Trait**（`Order` + `Debug` / `Send`） | ✅ **仍受三类约束** — 调用方对 std trait 有固定预期，违反会「猜不对」 |

```text
三类划分  ──针对──►  公开类型 × 标准库 Trait（Debug / Send / Copy …）
自定义 Trait（Strategy / OrderExecutor …）  ──►  按需设计，但公开 API 仍讲 Unsurprising
内部业务 struct  ──►  灵活；要给别人用或进 workspace 公共模块时再对齐三类
```

**例子**：`Strategy` trait 是领域抽象，不必硬套「Ⅰ 类几乎总实现」；但 `struct Order` 若发布给策略框架用，`#[derive(Debug, PartialEq, Eq)]` 和 `Send` 仍应按三类来 — 否则别人 `println!("{order:?}")` 或 `spawn` 传订单时会踩坑。

→ 自定义 trait 设计：[03 人体工程学 impl](./03-ergonomic-trait-implementations.md)

---

## 划分标准（三类边界）

核心看三件事：**实现后对类型的影响范围** · **副作用强度** · **违反约定的代价**。

| 类别 | Trait | 影响 / 副作用 | 代价 | 策略 |
|------|-------|---------------|------|------|
| **Ⅰ 几乎总实现** | `Debug` · `PartialEq` / `Eq` | **只增便利**，不改核心行为 | 不加 → 调试/测试/断言反直觉 | 公开类型**默认 derive** |
| **Ⅱ 线程默认契约** | `Send` · `Sync` | **内存安全底线**，非可选装饰 | 不满足 → 无法跨线程传策略/订单 | 多数**自动满足**；例外**文档 + 替代** |
| **Ⅲ 谨慎对待** | `Copy` · `Hash` | **长期锁死**或**强不变式** | Copy 难演进 · Hash 错配 map 失效 | **按需**、成对、小值才 Copy |

**派生宏的默认行为**与风险等级一致：`Debug` / `PartialEq` 可放心 `derive`；`Copy` **不会**自动派生、须显式加；`Hash` 作 map 键时须与 `Eq` **成对**。

→ 详表 + 量化场景 + 模板：[02-1 完整解读](./02-1-common-traits-full-guide.md)

---

## Ⅰ 几乎总是该有

| Trait | 理由 |
|-------|------|
| **`Debug`** | 日志、测试、`{:?}`；不改语义，只让排查省力 |
| **`PartialEq` / `Eq`** | 断言、业务相等；不加则订单/信号 struct 无法直接 `==` |

## Ⅱ 多线程默认假设

| Trait | 说明 |
|-------|------|
| **`Send`** | 所有权可跨线程转移 |
| **`Sync`** | `&T` 可跨线程共享 |

行情接收 → 策略计算 → 订单发送，链路默认要求 `Send`。`Rc` 等例外须在文档说明并用 `Arc` 等替代。

## Ⅲ 谨慎对待

| Trait | 注意 |
|-------|------|
| **`Copy`** | 一旦实现难撤回；加 `String` 字段会全链路编译失败；隐式复制有性能/语义代价 |
| **`Hash`** | 必须与 `Eq` 严格一致；键字段入 map 后不可静默修改 |

---

## 速记

**适用谁**：库作者最佳实践 · 内部业务灵活 · 自定义 trait 按需 · impl std trait 仍守三类

| 类 | Trait | 策略 |
|----|-------|------|
| Ⅰ | `Debug` · `PartialEq` / `Eq` | 默认 derive |
| Ⅱ | `Send` · `Sync` | 多数自动；例外文档 |
| Ⅲ | `Copy` · `Hash` | 按需 · 成对 · 小值才 Copy |

→ 完整解读：[02-1](./02-1-common-traits-full-guide.md) · ER [Item 10](../../01-ER/Chapter-02-Traits/Item-10-standard-traits/README.md)
