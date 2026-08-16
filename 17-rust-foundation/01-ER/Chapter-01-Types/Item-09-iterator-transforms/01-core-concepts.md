# Item 9 · 核心知识点

← [Item 9 目录](./README.md)

### `Iterator` trait

- 核心方法：**`next() -> Option<Item>`**（`None` 表示耗尽）。
- **`for item in iter`**：语法糖，反复 `next` 并绑定元素。

### 适配器（Adaptors，惰性）

| 类别 | 示例 |
|------|------|
| 流程 | `take`、`skip`、`chain`、`cycle` |
| 映射 | `map`、`cloned`、`enumerate`、`zip` |
| 过滤 | `filter`、`filter_map`、`flatten` |

### 消费者（Consumers，终止迭代）

`for_each`、`sum`、`fold`、`find`、`any`、`all`、**`collect`** 等。

### 三种迭代方式

| 写法 | 产出 | 集合 |
|------|------|------|
| `into_iter()` / `for x in v` | 按值 / 拥有项 | **消费** `v` |
| `iter()` / `for x in &v` | `&Item` | 保留 `v` |
| `iter_mut()` | `&mut Item` | 保留 `v`，可改元素 |

---
