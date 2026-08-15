# 3.2 Type System Guidance（类型系统引导）

> 所属：**Obvious** · [← 章索引](./README.md)

用类型把**正确用法**做成阻力最小的路径；错误用法尽量**无法编译**。

## 语义化类型

- 避免多个相邻 **`bool`** 参数 → 用 **`enum`** 或 newtype。
- 无效状态**不可表达** → [ER Item 01 · 设计模式](../../01-ER/Chapter-01-Types/Item-01-express-data-structures/06-design-patterns.md)

## 类型状态 (Typestate)

用 **ZST 阶段标记** 或不同 struct 状态，使非法操作无法构造：

```text
Rocket<Ready>  --launch()-->  Rocket<Launched>  --只有后者有 accelerate()
```

Book → [17.3 状态模式与博客工作流](../../00-Book/17-oop/17.3-状态模式与博客工作流.md)

## `#[must_use]`

对不应被忽略的返回值打标：

- `Result`、配置构建器、`Iterator` 适配器链
- 提醒调用方处理或显式 `let _ = ...`

## Builder

复杂构造 → [ER Item 07 Builder](../../01-ER/Chapter-01-Types/Item-07-builders/README.md)
