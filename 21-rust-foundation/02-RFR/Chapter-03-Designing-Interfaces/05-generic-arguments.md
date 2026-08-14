# 2.1 Generic Arguments（泛型参数）

> 所属：**Flexible** · [← 章索引](./README.md)

在不过度牺牲表达力的前提下，**少钉死具体类型**，把选择权交给调用方。

## 接受多种输入形态

优先 **`impl AsRef<str>`、`impl AsRef<Path>`** 等，而非仅 `&str`：

- 调用方可传 `String`、`&str`、`Cow<str>` 等。
- 内部统一 `.as_ref()` 取得借用视图。

## 泛型 vs `impl Trait`

| | 泛型 `T: Trait` | `impl Trait` |
|---|----------------|--------------|
| 调用方 | 单态化每份 T | 隐藏具体类型 |
| 二进制 | 可能膨胀 | 单份 API 表面 |

非热点路径可权衡；详见 [RFR 第 2 章 · 分发](../Chapter-02-Types/05-compilation-dispatch.md)。

## 约束粒度

- 只 bound **真正需要**的 trait；过宽 bound 限制 impl，过窄导致 API 难用。
- 关联类型 vs 泛型参数 → [06 泛型 Trait](../Chapter-02-Types/06-generic-traits.md)

Book → [10.1 泛型](../../00-Book/10-generics-traits-lifetimes/10.1-泛型数据类型.md) · ER → [Item 05 类型转换 · AsRef](../../01-ER/Chapter-01-Types/Item-05-type-conversions/README.md)
