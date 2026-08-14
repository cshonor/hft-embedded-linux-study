# 1.4 Dynamically Sized Types and Wide Pointers（DST 与宽指针）

> 所属：**Types in Memory** · [← 章索引](./README.md)

← [03 复合类型](./03-complex-types.md) · 下一节 → [05 编译与分发](./05-compilation-dispatch.md)

---

**DST** = 编译期 `size_of` 未知 → 只能经指针间接持有；**宽指针** = 数据地址 + 元数据（`len` 或 `vtable`）。

```text
切片 DST     &str / &[T]     第二字 = len
trait object &dyn Trait      第二字 = vtable
```

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **04.1** | DST 基础 · Sized vs DST | [04-1-dst-basics.md](./04-1-dst-basics.md) |
| **04.2** | 宽指针布局 · 实测 | [04-2-wide-pointers.md](./04-2-wide-pointers.md) |
| **04.3** | `dyn Trait` · vtable · 调用流程 | [04-3-dyn-vtable.md](./04-3-dyn-vtable.md) |
| **04.4** | Vec 句柄 · FFI · HFT | [04-4-containers-ffi-hft.md](./04-4-containers-ffi-hft.md) |
| — | 速记 · 自测 |

**建议阅读顺序**：`04.1` → `04.2` → `04.3` → `04.4`

---

## 一句话

> **瘦指针 8B；宽指针 16B（data + len/vtable）；`dyn` 走 vtable 动态分发。**

---

## 延伸阅读

- 对象安全 → [05.3](./05-3-object-safety.md)
- 关联类型与 `dyn Iterator<Item = T>` → [06](./06-generic-traits.md)
- 实测 → [`layout-demo`](./layout-demo/)
