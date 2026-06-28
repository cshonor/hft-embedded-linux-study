## 5. 捕获信号与用户态处理程序

> handler 在 **用户态** — 内核必须 **伪造栈帧** 再 **恢复**

---

### 一、问题

传递自定义 handler 时：

- 内核正在 **内核态**  
- handler 代码在 **用户态**  
- 被中断处需 **可恢复**  

→ 需在 **用户栈** 上构造特殊 **信号栈帧 (signal frame)**。

---

### 二、建立栈帧：`setup_frame()` / `setup_rt_frame()`

`do_signal()` 调用：

| 函数 | 场景 |
|------|------|
| **`setup_frame()`** | 传统信号帧 |
| **`setup_rt_frame()`** | 实时信号（`SA_SIGINFO`） |

在用户栈压入：

- **被保存的硬件上下文**（寄存器等）  
- **返回地址** — 指向 sigreturn 桩代码  
- （实时信号）附加 **siginfo** 等  

---

### 三、修改 EIP

内核修改保存在 **内核栈** 上的用户态 **指令指针 (EIP)**，使其指向 handler 入口。

进程 **返回用户态** 时 → 直接执行 handler，而非被中断的原指令。

---

### 四、handler 结束：`sigreturn`

handler 执行完毕后，栈帧中的返回地址引导至 **vsyscall 页** 的 **`__kernel_sigreturn`**：

| 系统调用 | 作用 |
|----------|------|
| **`sigreturn()`** | 传统帧恢复 |
| **`rt_sigreturn()`** | 实时信号帧恢复 |

将用户栈上保存的上下文 **复制回内核栈** → 原程序 **无缝继续**。

→ vsyscall / 快速 syscall：[Ch 10 section-4](../../chapter-10-system-calls/notes/section-4-进入与退出.md)

> **深潜可选：** `setup_rt_frame` 栈布局、`RESTORER` 与 `SA_RESTORER` — 见 `arch/x86/kernel/signal.c`。

---

← [4. 生成与传递](./section-4-生成与传递.md) · 下一节 [6. syscall 重启](./section-6-系统调用重启与相关syscall.md)
