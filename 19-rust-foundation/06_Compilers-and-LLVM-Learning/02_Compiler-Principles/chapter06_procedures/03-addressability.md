# 第 6 章 · 过程抽象 · §3 建立可寻址性

← [本章目录](./README.md) · 上一节：[02-activation-records.md](./02-activation-records.md) · 下一节：[04-parameter-passing.md](./04-parameter-passing.md)

---

生成代码时必须知道：**变量在内存哪里**。

---

## 局部变量

**当前 AR 基址（ARP / Frame Pointer）+ 固定偏移**

```text
local_x  @  [FP + 8]
local_y  @  [FP + 16]
```

- 偏移在编译期由符号表 + 布局 pass 确定。
- **LLVM**：常 lowering 为 `%rbp` 相对或 **SSA + alloca** 再 mem2reg。

→ RFR [03-2 alloca vs heap](../../../02-RFR/Chapter-01-Foundations/03-2-os-memory-layout.md)

---

## 非局部变量（嵌套过程）

**Pascal 式** — 内层过程访问**外层**局部变量（非全局）。

| 机制 | 说明 |
|------|------|
| **存取链（Access link / Static link）** | AR 中指针，沿**静态嵌套层次**向上找定义该变量的 AR |
| **全局显示（Display）** | 数组 `display[d]` = 第 d 层静态嵌套当前 AR；O(1) 查外层 |

```text
嵌套深度:  outer ──► middle ──► inner
inner 读 outer 的 x:  沿 static link 或 display[outer_depth] + offset
```

**C / Rust / Lox**：无 Pascal 式嵌套函数（Rust 有闭包但用 **捕获环境 / upvalue**）→ [clox ch25 upvalue](../../../01_Crafting-Interpreters/part03_clox/chapter25_objects/README.md)

---

## 全局 / 静态

- **静态数据区**固定地址，或 PC 相对（PIC）。
- **Rust `static`**：特殊段 + 生命周期 `'static`。

---

## 与 IR / 后端

寻址方式决定 **ILOC/机器码** 用基址+偏移还是绝对地址 — ch11 代码生成输入。
