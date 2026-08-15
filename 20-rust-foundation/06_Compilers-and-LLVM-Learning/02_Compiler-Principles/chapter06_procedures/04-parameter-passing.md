# 第 6 章 · 过程抽象 · §4 过程间传值

← [本章目录](./README.md) · 上一节：[03-addressability.md](./03-addressability.md) · 下一节：[05-call-linkages.md](./05-call-linkages.md)

---

## 参数传递

| 机制 | 行为 | 典型 |
|------|------|------|
| **值调用（Call-by-value）** | 传**副本**；被调改形参不影响实参 | C 默认、Rust 默认 |
| **引用调用（Call-by-reference）** | 传**地址**；被调通过指针改实参 | C++ `&`、Pascal `var` |

**Rust 补充**：

| | 说明 |
|---|------|
| **`&T` / `&mut T`** | 借用语义 — 编译期检查，运行时传指针 |
| **Move** | 所有权转移，非拷贝 |

---

## 返回值

| 方式 | 说明 |
|------|------|
| **寄存器** | 小返回值放 `rax` 等（SysV ABI） |
| **栈上 hidden pointer** | 大 struct 返回 — 调用者分配空间，传地址 |
| **AR 槽位** | 某些 ABI 指定返回区 |

被调 **Epilogue** 写返回值；调用者 **Postreturn** 从约定位置读取。

→ [§5 链接约定](./05-call-linkages.md)

---

## 与 clox

- **参数与局部共享栈窗口** — 零拷贝传递槽位。
- 返回值在 **栈顶** — VM 约定。

→ [ch24 Function calls](../../../01_Crafting-Interpreters/part03_clox/chapter24_calling-and-closures/README.md)

---

## 工程注意

| 话题 | 风险 |
|------|------|
| **值传递大 struct** | 拷贝开销 — Rust 用 move / 引用 |
| **引用 + 生命周期** | 悬垂 — `rustc` 拒绝；C 无检查 |
| **ABI 不一致** | FFI 必须 `repr(C)` 对齐约定 |
