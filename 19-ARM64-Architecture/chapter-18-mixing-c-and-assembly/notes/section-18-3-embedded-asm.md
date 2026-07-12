## §18.3 内嵌汇编 (Embedded Assembler)

> **Ch 18 · C 与汇编混合编程** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)  
> 目录文件名 `embedded-asm`；本书称 **Embedded Assembler**（内嵌汇编）。

---

### 定位

**比 inline 更大、比独立 `.s` 更贴 C 模块：**

```c
__asm int asm_strcpy(char *dst, const char *src);
// 随后在 __asm 函数体 { … } 或单独 asm 块实现
```

| 属性 | 说明 |
|------|------|
| **完整指令集** | **ARM + Thumb** 均可（同模块混用 — [Ch17](../chapter-17-arm-thumb-thumb2-instructions/)） |
| **不内联** | 生成 **独立符号** — 正常 **`BL` 调用开销** |
| **C 原型** | 参数/返回值类型 — 链接器按 **AAPCS** 对接 |

**适用：** **strcpy 级** 手写循环 · **VCVT 定点↔浮点** 库 · 任何需 **全 asm 优化** 的函数。

---

### 语法与 C 的差异（坑）

| 点 | 内嵌 asm | C |
|----|----------|---|
| **表达式** | **始终无符号** | 有符号/无符号规则 |
| **前导零** | **`010` = 十进制 10** | C 中 **`010` = 八进制 8** |
| **运算符优先级** | **汇编规则** | C 规则 |

**口述：** 复制 C 表达式进 asm **会 silent bug** — 常数用 **十六进制** 最安全。

---

### 程序员责任（编译器不帮）

| 项 | 要求 |
|----|------|
| **返回** | **必须手写 `BX lr`**（或 **`POP {…, pc}`**）— 编译器 **不生成 return** |
| **AAPCS** | **r4–r11** 用了必压栈 · **r0–r3** 参返 · **SP 8 对齐** |
| **合规检查** | **无** — 错帧 = 调用者寄存器损坏 |

→ [Ch13 §13.3 序言/尾声](../chapter-13-subroutines-stacks/notes/section-13-3-subroutines.md)

**Embedded 函数模板：**

```asm
__asm int foo(int x)
{
    ; 若用 r4+
    PUSH    {r4, lr}
    ; … x 已在 r0 …
    MOV     r0, #result
    POP     {r4, pc}        ; 或 BX lr 若未 push lr 到栈上 pc
}
```

（精确序言依是否 **修改 lr** 调整 — 同 Ch13。）

---

### vs 独立 `.S` 文件

| | **Embedded（同 .c）** | **foo.S + extern** |
|---|----------------------|---------------------|
| 工具链 | ARMCC **内嵌语法** | **GNU as** · **`.global foo`** |
| 可见性 | 模块内 | 全链接单元 |
| Linux 内核 | 较少 | **常见**（`arch/arm/lib/*.S`） |

**GNU 路线：** `void foo(void);` + `arch/arm/foo.S` 实现 — 语义同 embedded，语法不同。

---

### 可复述要点

1. Embedded = **全 ISA 的 asm 函数**，**独立 BL 调用**。  
2. **自己写返回 + AAPCS** — 编译器 **不检查**。  
3. **常量/优先级** 按 **汇编规则** — 别照抄 C 字面。
