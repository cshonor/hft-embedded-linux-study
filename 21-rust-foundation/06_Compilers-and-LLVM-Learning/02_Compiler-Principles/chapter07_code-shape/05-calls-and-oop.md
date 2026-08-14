# 第 7 章 · 代码形态 · §5 过程调用与 OOP 扩展

← [本章目录](./README.md) · 上一节：[04-control-flow.md](./04-control-flow.md) · 下一节：---

落实 [ch6 过程抽象](../chapter06_procedures/README.md) 的**具体 IR / 机器码形态**。

---

## 过程调用

| 步骤 | 代码形态 |
|------|----------|
| **实参求值** | 按 ABI 顺序计算，放寄存器或栈 |
| **过程值** | 函数指针 / 闭包环境指针 — 额外参数 |
| **保存寄存器** | caller-saved 在 precall spill |
| **call 指令** | 直接 / 间接调用 |
| **返回后** | postreturn 取返回值、清栈 |

→ [ch6 §5 precall/prologue/epilogue/postreturn](../chapter06_procedures/05-call-linkages.md)

**clox**：`OP_CALL` + 栈窗口 — 形态简化但结构同构。

**Rust**：单态调用可 **devirtualize + inline**；`Fn` trait 对象 = 间接调用形态。

---

## 面向对象扩展

| 特性 | 代码形态 |
|------|----------|
| **单一继承** | 对象 record：字段 + vptr；字段偏移固定 |
| **方法分发** | **虚函数表（vtable）** — `obj.vptr->method[idx]` |
| **super 调用** | 静态偏移 + 父类 vtable 槽位 |

```text
call  obj->vptr[slot](obj, args…)
```

**C++**：标准 vtable 布局（Itanium ABI 等）。  
**Java**：interface 默认方法、invokedynamic 等变体。  
**clox**：**方法 + 绑定接收者** → [ch28 Methods](../../../01_Crafting-Interpreters/part03_clox/chapter28_methods/README.md)

**Rust**：无继承；`dyn Trait` ≈ fat pointer (data, vtable) — 同类 dispatch 形态。

---

## 与优化 / 后端

| 形态 | 优化机会 |
|------|----------|
| **直接静态调用** | 内联、IPO |
| **间接 / 虚调用** | 去虚化（devirtualization）、PGO |
| **大 struct 传值** | 改传指针 / sret — ABI 形态 |

ch11 **指令选择** 在 ABI + 调用形态约束下选 `call` / `tail call` 等。

---

## Part II 收束

| 章 | 贡献 |
|:--:|------|
| **5** | IR 语言 |
| **6** | 运行时模型 |
| **7** | 源结构 → IR 的**具体形态** |

→ **Part III ch8** 在这套形态上做**改进**（优化）。
