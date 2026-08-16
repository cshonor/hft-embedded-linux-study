# 第 23 章 · Jumping Back and Forth（来回跳转） · §23.1 If 语句（If Statements）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-logical-operators.md](./02-logical-operators.md)

---

**语义**：condition falsy → 跳过 then；可选 else。

**指令**：

| Opcode | 行为 |
|--------|------|
| **`OP_JUMP_IF_FALSE offset`** | pop 条件；若 **falsy** → **`ip += offset`** |
| **`OP_JUMP offset`** | 无条件 **`ip += offset`** |

**编译顺序（有 else）示意**：

```text
compile condition
JUMP_IF_FALSE → else分支
compile then
JUMP → 结束
else:
compile else
结束:
```

### 回填（Backpatching）

emit **`JUMP_*`** 时 **还不知道** 要跳过多少字节 → 先写 **占位 offset（如 0xFF）**，记录 **patch 地址**；分支编译完后再 **写回真实偏移**。

| API | 作用 |
|-----|------|
| **`emitJump(op)`** | 发 opcode + 占位 2 字节 offset，返回 patch 点 |
| **`patchJump(offset)`** | `当前ip - offset - 2` 写入真实跳转距离 |

**offset 单位**：通常为 **字节数**（跳过多少 code 字节），非「指令条数」。

**对照 jlox [ch9 §9.2](../../part02_jlox/chapter09_control-flow/README.md)**：悬挂 else 靠 Parser；clox 靠 **跳转布局 + 回填**。

---
