## 4. 执行跟踪 (Execution Tracing)

> **`ptrace()`** — gdb/strace 的底层

---

### 一、基本能力

一个进程（**追踪者**）可 **监控 / 控制** 另一个进程（**被追踪者**）：

| 操作 | 效果 |
|------|------|
| **`PTRACE_ATTACH`** | 附加 — 改被追踪进程 **`parent`** 指向追踪者 |
| 读写内存/寄存器 | 检查状态 |
| 继续 / 单步 | 控制执行流 |

→ syscall：[Ch 10](../chapter-10-system-calls/) · 信号 SIGCHLD：[Ch 11](../chapter-11-signals/)

---

### 二、单步：`PTRACE_SINGLESTEP`

x86 实现：

1. 内核置 **`eflags.TF`（Trap Flag）**  
2. CPU **每条指令** 后触发 **Debug 异常**  
3. 内核截获 → 向调试器发 **`SIGCHLD`**  

调试器在信号处理中检查子进程状态。

→ Debug 异常：[Ch 4](../chapter-04-interrupts-and-exceptions/)

---

### 三、HFT 注意

- **ptrace 附加** 会 **暂停** 目标 — 生产热路径 **禁用** 或避免  
- `strace`/`gdb` 引入 **巨大开销**

---

← [3. 内存布局](./section-3-地址空间与内存布局.md) · 下一节 [5. 可执行格式](./section-5-可执行格式与执行域.md)
