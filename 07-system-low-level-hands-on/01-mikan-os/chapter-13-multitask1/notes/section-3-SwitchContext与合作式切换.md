## 3. SwitchContext 与合作式切换

---

### 一、TaskContext 结构体

内存中 **保存/恢复 CPU 状态** 的 C 结构 — 字段对齐汇编布局：

```cpp
struct TaskContext {
    uint64_t cr3;      // 本章常共用 identity 页表
    uint64_t rbp, r15, …, rbx;  // callee-saved 等
    uint64_t rip, rsp;
    uint64_t rflags;
    // …
};
```

每个 **Task** 持有一个 **TaskContext** + **独立栈数组**。

---

### 二、SwitchContext() 汇编核心

```asm
SwitchContext:
    ; 保存 old_ctx 指向的当前寄存器
    mov [old_ctx + OFF_RBX], rbx
    ...
    pushfq
    pop [old_ctx + OFF_RFLAGS]
    ; 保存 RIP/RSP — 常配合 call 返回地址入栈

    ; 从 new_ctx 加载寄存器
    mov rbx, [new_ctx + OFF_RBX]
    ...
    push qword [new_ctx + OFF_RFLAGS]
    popfq
    ; 跳转 new_ctx->rip，设置 new_ctx->rsp
```

| 指令 | 作用 |
|------|------|
| **`mov`** | 寄存器 ↔ 内存 |
| **`pushfq` / `popfq`** | 保存/恢复 **RFLAGS** |
| **`call` / `iret` 栈帧** | 构造 **可返回的 RIP/RSP** — 从「上次停处」续跑 |

**`iret` 特性（书中技巧）：** 利用 **中断返回栈帧** 格式 — **RIP/CS/RFLAGS/RSP/SS** 一次恢复。

---

### 三、合作式（Cooperative）多任务

**切换时机：** Task **主动** 调用 `SwitchContext(next)` — 如 `yield()`。

| 优点 | 缺点 |
|------|------|
| 实现 **相对简单** — 切换点已知 | Task **死循环/bug** → **永不 yield** → **全 OS 冻结** |
| 无需定时器抢占 | 不适合 **公平调度** |

**结论：** 必须升级 **抢占式**（§4）。

→ [01 Day 15 多任务](../../02-30days-os/day-15-multitask1/)

---

← [2. 上下文](./section-2-多任务与上下文.md) · 下一节 [4. 抢占](./section-4-抢占式多任务与时间片.md)
