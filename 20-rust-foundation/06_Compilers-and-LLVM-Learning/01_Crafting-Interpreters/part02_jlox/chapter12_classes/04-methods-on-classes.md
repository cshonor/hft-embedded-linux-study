# 第 12 章 · Classes（类） · §12.4 类上的方法（Methods on Classes）

← [本章目录](./README.md) · 上一节：[03-properties-on-instances.md](./03-properties-on-instances.md) · 下一节：[05-this.md](./05-this.md)

---

- 方法在类声明体中定义为 **`fun`-风格**，存于 **`LoxClass`**。
- **Get `obj.method`**：
  - 若 **fields** 有同名键 → **字段优先**（覆盖方法名）。
  - 否则取类方法 → 包装为 **`LoxBoundMethod(instance, method)`**。

**`LoxBoundMethod`**：

| 实现 | 说明 |
|------|------|
| **`LoxCallable`** | 调用时执行底层 **`LoxFunction`** |
| 绑定 | 携带 **`this` 实例**（§12.5） |

用户代码：`bacon.cook()` → Get 得到 bound method → Call。

---
