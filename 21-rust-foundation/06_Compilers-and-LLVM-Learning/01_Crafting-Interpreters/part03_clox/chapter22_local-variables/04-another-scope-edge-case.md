# 第 22 章 · Local Variables（局部变量） · §22.5 另一个作用域边缘情况（Another Scope Edge Case）

← [本章目录](./README.md) · 上一节：[03-using-locals.md](./03-using-locals.md) · 下一节：[05-ast.md](./05-ast.md)

---

**问题**：`var a = a;`

- 右侧 `a` 应是 **外层** 的 `a`，不是 **正在声明、尚未初始化** 的内层槽。
- 若声明后立即允许读同一 slot → **读到未定义 / nil 错误**。

**两阶段**：

| 阶段 | 行为 |
|------|------|
| **`addLocal`** | 占用 slot，**depth 仍标记「未初始化」** |
| compile initializer | 允许 resolve **外层** `a`（内层槽不可见） |
| **`markInitialized`** | 初始化完成后才允许 **GET/SET 本 slot** |

**`resolveLocal`**：若 local 的 **`depth == -1`（未 init）** 且正在读变量 → **跳过**，继续向外找。

**对照**：TDZ（Temporal Dead Zone）的简化版语义；Rust 也有「声明 vs 初始化」规则。

---
