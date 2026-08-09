## 3. 中断描述符表 (IDT) 与三种门

> 内核允许硬件中断前，必须先初始化 **IDT** — 向量号 → 处理程序

---

### 一、IDT 作用

每个 **中断/异常向量号** 映射到对应的 **处理程序入口**（通过门描述符）。

---

### 二、三种门 (Gate Descriptors)

| 门类型 | 行为 | 用途 |
|--------|------|------|
| **中断门 (Interrupt gate)** | 进入时 **禁用可屏蔽中断** | 仅内核态 ISR |
| **陷阱门 (Trap gate)** | **不改变** IF（中断使能标志） | 异常、需要嵌套的场景 |
| **系统门 (System gate)** | 允许 **用户态** 进入的陷阱门 | `int $0x80` 等 **系统调用** |

---

### 三、和系统调用的关系

2.6 时代用户态通过 **`int $0x80`** 触发编程异常 → 经 **系统门** 进内核。

Modern x86-64 多用 **`syscall/sysenter`** 指令，概念相同：**用户态 → 内核态入口**。

→ 深潜：[Ch 10 系统调用](../chapter-10-system-calls.md) · [08 TLPI](../../../03-linux-userspace-api/)

### 常见陷阱

1. 把 ULK 的 8 字节门描述符用于 64 位——x86-64 IDT 条目是 16 字节，含 IST 字段和新的属性位
2. 以为 `int $0x80` 还是现代 syscall 入口——x86-64 用 `syscall` 指令 + MSR_LSTAR，`int $0x80` 保留但慢且不推荐
3. 混淆中断门和陷阱门——中断门自动关 IF（CLI），陷阱门不关，syscall 指令用专用机制

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86-64 IDT 条目和 32 位有什么区别？

<details><summary>答案</summary>

32 位：8 字节，`offset[31:0]` 拆成两段。64 位：16 字节，`offset[63:0]` 拆成三段，新增 IST 字段（3 位，指定专用栈），`type` 字段中中断门=0xE，陷阱门=0xF。64 位 IDT 条目还包含 `CS` 选择符（必须指向 64 位代码段）。ULK 的 8 字节格式不能用于 64 位分析。

</details>

**Q2.** IST（Interrupt Stack Table）解决什么问题？

<details><summary>答案</summary>

某些异常（double fault, NMI, machine check）在当前栈损坏时无法处理。IST 让这些异常切换到预定义的专用内核栈（TSS 中的 `IST[n]` 指针），保证有干净的栈可用。典型场景：内核栈溢出 → #DF → 用 IST 栈处理 → panic 而非 triple fault。

</details>

**Q3.** `syscall` 指令相比 `int $0x80` 有什么优势？

<details><summary>答案</summary>

① 不走 IDT 查表，直接从 `MSR_LSTAR` 加载入口地址（更快）。② 不压入 error code/SS/CS 的旧式帧，只存 `RIP` 到 `RCX`、`RFLAGS` 到 `R11`。③ 自动切换 `CS`/`SS` 到内核段（从 MSR 加载）。④ 不修改 IF 标志（中断保持开启）。实测 `syscall` 比 `int $0x80` 快 ~3-5 倍。

</details>

</details>

---

← [2. 分类](./section-2-中断与异常分类.md) · 下一节 [4. 控制路径嵌套](./section-4-控制路径嵌套.md)
> ↔ [LKD Ch07 §7.6 中断处理机制的实现](../../../05-linux-kernel/00_Book_3rd_Notes/chapter-07-interrupts/notes/section-7.6-中断处理机制的实现.md)
