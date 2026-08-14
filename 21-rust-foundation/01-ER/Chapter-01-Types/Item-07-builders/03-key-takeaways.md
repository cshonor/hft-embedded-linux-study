# Item 7 · 重点结论

← [Item 7 目录](./README.md)

| | **消费型（Consuming）** | **可变借用型（Mutating）** |
|--|-------------------------|----------------------------|
| setter 签名 | `mut self` → `Self` | `&mut self` → `&mut Self` |
| 链式调用 | ✅ 一气呵成 | ⚠️ 临时值链式会踩生命周期 |
| `if` 分支 | 需 `builder = builder.x()` 重绑 | ✅ `builder.x()` 直接改 |
| `build` | 通常 `build(self)` 消耗 builder | 常 `build(&self)`，可 **Clone 后多次 build** |
| 重复 build | ❌ 一次 `build` 吃掉 builder | ✅ 同一模板可多次产出 |

### 生态

- 手写 Builder 把样板从**调用方**挪到 **Builder 定义**；
- 优先用 **`derive_builder`** 等 crate 自动生成。

---
